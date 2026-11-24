#define ZEE_INPUT_TEST_ENTRY original_input_fixture_main
#include "test_zeemote_input.c"
int main(void){
 assert(original_input_fixture_main()==0);setup();zee_input_clear();time_us=100;
 assert(!zee_input_session_live());ZeeState neutral={.lx=128,.ly=128,.rx=128,.ry=128};zee_input_update(&neutral);assert(zee_input_session_live());
 time_us+=1499999;assert(zee_input_session_live());time_us++;assert(!zee_input_session_live());
 zee_input_keepalive();assert(zee_input_session_live());zee_input_clear();assert(!zee_input_session_live());zee_input_keepalive();assert(!zee_input_session_live());
 time_us=0xfffffff0u;zee_input_update(&neutral);time_us+=1000;assert(zee_input_session_live());time_us+=1600000;assert(!zee_input_session_live());
 zee_input_update_kind(&neutral,1);assert(zee_input_session_live());zee_input_clear();assert(!zee_input_session_live());
 puts("PASS: neutral/live session, exact stale boundary, trusted keepalive, disconnect, reconnect and clock wrap");return 0;
}
