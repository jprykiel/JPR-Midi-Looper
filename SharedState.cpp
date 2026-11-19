#include "SharedState.h"

SharedState& SharedState::get()
{
    static SharedState state;
    return state;
}
