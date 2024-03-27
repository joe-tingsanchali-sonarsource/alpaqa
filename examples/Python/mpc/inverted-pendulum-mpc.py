# %% Model

import casadi as cs
import numpy as np

# State vector
θ = cs.SX.sym("θ")  # Pendulum angle                     [rad]
ω = cs.SX.sym("ω")  # Pendulum angular velocity          [rad/s]
x = cs.SX.sym("x")  # Cart position                      [m]
v = cs.SX.sym("v")  # Cart velocity                      [m/s]
state = cs.vertcat(θ, ω, x, v)  # Full state vector
nx = state.shape[0]  # Number of states

# Input
F = cs.SX.sym("F")  # External force applied to the cart [N]
nu = F.shape[0]  # Number of inputs

# Parameters
F_max = 5  #        Maximum force applied to the cart    [N]
m_cart = 0.8  #     Mass of the cart                     [kg]
m_pend = 0.3  #     Mass of the pendulum                 [kg]
b_cart = 0.1  #     Friction coefficient of the cart     [N/m/s]
l_pend = 0.3  #     Length of the pendulum               [m]
g_gravity = 9.81  # Gravitational acceleration           [m/s²]
Ts = 0.025  #       Simulation sampling time             [s]
N_horiz = 50  #     MPC horizon                          [time steps]
N_sim = 300  #      Simulation length                    [time steps]

# Continous-time dynamics
a_pend = l_pend * cs.sin(θ) * ω**2 - g_gravity * cs.cos(θ) * cs.sin(θ)
a = (F - b_cart * v + m_pend * a_pend) / (m_cart + m_pend)
α = (g_gravity * cs.sin(θ) - a * cs.cos(θ)) / l_pend
f_c = cs.Function("f_c", [state, F], [cs.vertcat(ω, α, v, a)])

# 4th order Runge-Kutta integrator
k1 = f_c(state, F)
k2 = f_c(state + Ts * k1 / 2, F)
k3 = f_c(state + Ts * k2 / 2, F)
k4 = f_c(state + Ts * k3, F)

# Discrete-time dynamics
f_d_expr = state + (Ts / 6) * (k1 + 2 * k2 + 2 * k3 + k4)
f_d = cs.Function("f", [state, F], [f_d_expr])


# %% Model predictive control

# MPC inputs and states
mpc_x0 = cs.MX.sym("x0", nx)  # Initial state
mpc_u = cs.MX.sym("u", (1, N_horiz))  # Inputs
mpc_x = f_d.mapaccum(N_horiz)(mpc_x0, mpc_u)  # Simulated states

# MPC cost
Q = cs.MX.sym("Q", nx)  # Stage state cost
Qf = cs.MX.sym("Qf", nx)  # Terminal state cost
R = cs.MX.sym("R", nu)  # Stage input cost
s, u = cs.MX.sym("s", nx), cs.MX.sym("u", nu)
sin_s = cs.vertcat(cs.sin(s[0] / 2), s[1:])  # Penalize sin(θ/2), not θ
stage_cost_x = cs.Function("lx", [s, Q], [cs.dot(sin_s, cs.diag(Q) @ sin_s)])
terminal_cost_x = cs.Function("lf", [s, Qf], [cs.dot(sin_s, cs.diag(Qf) @ sin_s)])
stage_cost_u = cs.Function("lu", [u, R], [cs.dot(u, cs.diag(R) @ u)])

mpc_param = cs.vertcat(mpc_x0, Q, Qf, R)
mpc_y_cost = cs.sum2(stage_cost_x.map(N_horiz - 1)(mpc_x[:, :-1], Q))
mpc_u_cost = cs.sum2(stage_cost_u.map(N_horiz)(mpc_u, R))
mpc_terminal_cost = terminal_cost_x(mpc_x[:, -1], Qf)
mpc_cost = mpc_y_cost + mpc_u_cost + mpc_terminal_cost

# Box constraints on the actuator force:
C = -F_max * np.ones(N_horiz), +F_max * np.ones(N_horiz)

# Initial state of the system
state_0 = np.array([-np.pi, 0, 0.2, 0])

# Initial parameter value
Q = [10, 1e-2, 1, 1e-2]
Qf = np.array([10, 1e-1, 1000, 1e-1])
R = [1e-1]
param_0 = np.concatenate((state_0, Q, Qf, R))

# Compile into an alpaqa problem
import alpaqa

# Generate C code for the cost function, compile it, and load it as an
# alpaqa problem description:
problem = (
    alpaqa.minimize(mpc_cost, cs.vec(mpc_u))  # objective and variables
    .subject_to_box(C)  #                       box constraints on the variables
    .with_param(mpc_param, param_0)  #          parameters to be changed later
    .with_name(f"inverted_pendulum_{N_horiz}")  # name used in generated files
).compile(sym=cs.MX.sym)

from datetime import timedelta

# Configure an alpaqa solver:
Solver = alpaqa.PANOCSolver
inner_solver = Solver(
    panoc_params={
        "max_time": timedelta(seconds=Ts),
        "max_iter": 200,
        "stop_crit": alpaqa.PANOCStopCrit.ProjGradUnitNorm2,
    },
    direction=alpaqa.StructuredLBFGSDirection(
        lbfgs_params={"memory": 15},
        direction_params={"hessian_vec_factor": 0},
    ),
)
alm_params = {
    "tolerance": 1e-4,
    "initial_tolerance": 1e-4,
}
solver = alpaqa.ALMSolver(
    alm_params=alm_params,
    inner_solver=inner_solver,
)


# %% Controller class


# Wrap the solver in a class that solves the optimal control problem at each
# time step, implementing warm starting:
class MPCController:
    def __init__(self, problem: alpaqa.CasADiProblem):
        self.problem = problem
        self.tot_it = 0
        self.tot_time = timedelta()
        self.max_time = timedelta()
        self.failures = 0
        self.u = np.ones(problem.n)

    def __call__(self, state_n):
        state_n = np.array(state_n).ravel()
        # Set the current state as the initial state
        self.problem.param[:nx] = state_n
        # Shift over the previous solution by one time step
        self.u = np.concatenate((self.u[nu:], self.u[-nu:]))
        # Solve the optimal control problem
        # (warm start using the shifted previous solution)
        self.u, _, stats = solver(self.problem, self.u)
        # Print some solver statistics
        print(
            f"{stats['status']!s:24} iter={stats['inner']['iterations']:4} "
            f"time={stats['elapsed_time']} "
            f"failures={stats['inner_convergence_failures']} "
            f"tol={stats['ε']:1.3e} stepsize={stats['inner']['final_γ']:1.3e}"
        )
        self.tot_it += stats["inner"]["iterations"]
        self.failures += stats["status"] != alpaqa.SolverStatus.Converged
        self.tot_time += stats["elapsed_time"]
        self.max_time = max(self.max_time, stats["elapsed_time"])
        # Return the optimal control signal for the first time step
        return self.u[:nu]


# %% Simulate the system using the MPC controller

# Simulation
mpc_states = np.empty((nx, N_sim + 1))
mpc_inputs = np.empty((nu, N_sim))
mpc_states[:, 0] = state_0
controller = MPCController(problem)
for n in range(N_sim):
    # Solve the optimal control problem:
    mpc_inputs[:, n] = controller(mpc_states[:, n])
    # Apply the first optimal control input to the system and simulate for
    # one time step, then update the state:
    mpc_states[:, n + 1] = f_d(mpc_states[:, n], mpc_inputs[:, n]).T

print(f"{controller.tot_it} iterations, {controller.failures} failures")
print(
    f"time: {controller.tot_time} (total), {controller.max_time} (max), "
    f"{controller.tot_time / N_sim} (avg)"
)


# %% Visualize the results

import matplotlib.pyplot as plt
import matplotlib as mpl
from matplotlib import animation

mpl.rcParams["animation.frame_format"] = "svg"

# Plot the cart and pendulum
fig, ax = plt.subplots()
h = 0.04
x = [state_0[2], state_0[2] + l_pend * np.sin(state_0[0])]
y = [h, h + l_pend * np.cos(state_0[0])]
(target_pend,) = ax.plot(x, y, "--", color="tab:blue", label="Initial state")
state_N = [0, 0, 0, 0]
x = [state_N[2], state_N[2] + l_pend * np.sin(state_N[0])]
y = [h, h + l_pend * np.cos(state_N[0])]
(target_pend,) = ax.plot(x, y, "--", color="tab:green", label="Target state")
(pend,) = ax.plot(x, y, "-o", color="tab:orange", alpha=0.9, label="MPC")
cart = plt.Rectangle((-2 * h, 0), 4 * h, h, color="tab:orange")
ax.add_patch(cart)
plt.legend()
plt.ylim([-l_pend + h, l_pend + 2 * h])
plt.xlim([-1.5 * l_pend, +1.5 * l_pend])
plt.gca().set_aspect("equal", "box")
plt.tight_layout()


class Animation:
    def __call__(self, n):
        state_n = mpc_states[:, n]
        x = [state_n[2], state_n[2] + l_pend * np.sin(state_n[0])]
        y = [h, h + l_pend * np.cos(state_n[0])]
        pend.set_xdata(x)
        pend.set_ydata(y)
        cart.set_x(state_n[2] - 2 * h)
        return [pend, cart]


ani = animation.FuncAnimation(
    fig, Animation(), interval=1000 * Ts, blit=True, repeat=True, frames=N_sim
)

fig, axs = plt.subplots(5, sharex=True, figsize=(6, 9))
ts = np.arange(N_sim + 1) * Ts
labels = [
    "Angle $\\theta$ [rad]",
    "Angular velocity $\\omega$ [rad/s]",
    "Position $x$ [m]",
    "Velocity $v$ [m/s]",
]
for i, (ax, lbl) in enumerate(zip(axs[:-1], labels)):
    ax.plot(ts, mpc_states[i, :])
    ax.set_title(lbl)
ax = axs[-1]
ax.plot(ts[:N_sim], mpc_inputs.T)
ax.set_title("Control input $F$ [N]")
ax.set_xlabel("Simulation time $t$ [s]")
ax.set_xlim(0, N_sim * Ts)
plt.tight_layout()

# Show the animation
plt.show()
