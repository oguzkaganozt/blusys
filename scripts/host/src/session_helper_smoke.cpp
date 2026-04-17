#include "blusys/framework/services/internal/session_helper.h"
#include "blusys/framework/test/test.hpp"

#include <cstdio>

namespace {

void test_connect_success()
{
    blusys_session_t session{};
    blusys_session_init(&session);

    BLUSYS_TEST_REQUIRE(blusys_session_request_connect(&session));
    BLUSYS_TEST_REQUIRE(blusys_session_is_connecting(&session));
    BLUSYS_TEST_REQUIRE(blusys_session_finish_connect(&session, true));
    BLUSYS_TEST_REQUIRE(blusys_session_is_connected(&session));
    BLUSYS_TEST_REQUIRE(!blusys_session_is_connecting(&session));
}

void test_connect_timeout()
{
    blusys_session_t session{};
    blusys_session_init(&session);

    BLUSYS_TEST_REQUIRE(blusys_session_request_connect(&session));
    BLUSYS_TEST_REQUIRE(!blusys_session_finish_connect(&session, false));
    BLUSYS_TEST_REQUIRE(!blusys_session_is_connected(&session));
    BLUSYS_TEST_REQUIRE(!blusys_session_is_connecting(&session));
}

void test_disconnect_while_inflight()
{
    blusys_session_t session{};
    blusys_session_init(&session);

    BLUSYS_TEST_REQUIRE(blusys_session_request_connect(&session));
    BLUSYS_TEST_REQUIRE(blusys_session_request_disconnect(&session));
    BLUSYS_TEST_REQUIRE(blusys_session_is_closing(&session));
    BLUSYS_TEST_REQUIRE(!blusys_session_finish_connect(&session, true));
    BLUSYS_TEST_REQUIRE(!blusys_session_is_connected(&session));
    blusys_session_finish_disconnect(&session);
    BLUSYS_TEST_REQUIRE(!blusys_session_is_closing(&session));
}

void test_reconnect_after_disconnect()
{
    blusys_session_t session{};
    blusys_session_init(&session);

    BLUSYS_TEST_REQUIRE(blusys_session_request_connect(&session));
    BLUSYS_TEST_REQUIRE(blusys_session_finish_connect(&session, true));
    BLUSYS_TEST_REQUIRE(blusys_session_is_connected(&session));

    BLUSYS_TEST_REQUIRE(blusys_session_request_disconnect(&session));
    blusys_session_finish_disconnect(&session);
    BLUSYS_TEST_REQUIRE(!blusys_session_is_connected(&session));
    BLUSYS_TEST_REQUIRE(!blusys_session_is_closing(&session));

    BLUSYS_TEST_REQUIRE(blusys_session_request_connect(&session));
    BLUSYS_TEST_REQUIRE(blusys_session_finish_connect(&session, true));
    BLUSYS_TEST_REQUIRE(blusys_session_is_connected(&session));
}

void test_worker_shutdown_flag()
{
    blusys_session_t session{};
    blusys_session_init(&session);

    blusys_session_set_worker_active(&session, true);
    BLUSYS_TEST_REQUIRE(blusys_session_request_disconnect(&session));
    BLUSYS_TEST_REQUIRE(blusys_session_is_closing(&session));
    blusys_session_finish_disconnect(&session);
    BLUSYS_TEST_REQUIRE(!blusys_session_is_worker_active(&session));
    BLUSYS_TEST_REQUIRE(!blusys_session_is_closing(&session));
}

}  // namespace

int main()
{
    test_connect_success();
    test_connect_timeout();
    test_disconnect_while_inflight();
    test_reconnect_after_disconnect();
    test_worker_shutdown_flag();

    std::fprintf(stdout, "session_helper_smoke: all asserts passed\n");
    return 0;
}
