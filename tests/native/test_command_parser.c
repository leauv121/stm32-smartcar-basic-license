#include "app/command_parser.h"
#include "test_assert.h"

static void assert_command(const char *text, AppCommandType type, AppMode mode)
{
    AppCommand command;
    TEST_ASSERT_EQ_INT(1, CommandParser_Parse(text, &command));
    TEST_ASSERT_EQ_INT(type, command.type);
    TEST_ASSERT_EQ_INT(mode, command.mode);
}

int main(void)
{
    AppCommand command;

    assert_command("MODE=NORMAL\r\n", APP_COMMAND_SET_MODE, APP_MODE_NORMAL);
    assert_command(" mode=circle ", APP_COMMAND_SET_MODE, APP_MODE_CIRCLE);
    assert_command("MODE=EIGHT", APP_COMMAND_SET_MODE, APP_MODE_EIGHT);
    assert_command("MODE=STABLE", APP_COMMAND_SET_MODE, APP_MODE_STABLE);
    assert_command("MODE=CHALLENGE", APP_COMMAND_SET_MODE, APP_MODE_CHALLENGE);
    assert_command("START", APP_COMMAND_START, APP_MODE_NONE);
    assert_command("STOP", APP_COMMAND_STOP, APP_MODE_NONE);
    assert_command("STATUS?", APP_COMMAND_STATUS, APP_MODE_NONE);
    assert_command("ERRORS?", APP_COMMAND_ERRORS, APP_MODE_NONE);
    assert_command("ACK", APP_COMMAND_ACK, APP_MODE_NONE);
    TEST_ASSERT_EQ_INT(0, CommandParser_Parse("MODE=UNKNOWN", &command));
    return test_failures;
}
