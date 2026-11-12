/* Preserve the native callback-mode target as Allegrex sixth argument t1. */
.set noreorder
.text
.align 2
.globl completion_disconnect_bridge
.type completion_disconnect_bridge, @function
.ent completion_disconnect_bridge
completion_disconnect_bridge:
 move $t1, $s2
 move $t2, $s4
 j completion_disconnect_receive
 nop
.end completion_disconnect_bridge
.size completion_disconnect_bridge, .-completion_disconnect_bridge

/* Native walker retains allocation-entry pointer in v1. Pass ctx as fifth
 * EABI register argument, preserving original callback in a3 and native ra. */
.globl transition_control_bridge
.type transition_control_bridge, @function
.ent transition_control_bridge
transition_control_bridge:
 addiu $t0, $v1, 4
 j transition_control_receive
 nop
.end transition_control_bridge
.size transition_control_bridge, .-transition_control_bridge
