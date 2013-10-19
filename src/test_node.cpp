#include <eixx/alloc_std.hpp>
#include <eixx/eixx.hpp>
#include <functional>
#include <iostream>

#define BOOST_REQUIRE

#include <eixx/connect/test_helper.hpp>

using namespace EIXX_NAMESPACE;

void usage(char* exe) {
    printf("Usage: %s -n NODE -r REMOTE_NODE [-c COOKIE] [-v VERBOSE] [-t RECONNECT_SECS]\n"
           "    -v VERBOSE          - verboseness: none|debug|message|wire|trace\n"
           "    -t RECONNECT_SECS   - reconnect timeout between reconnect attempts\n"
           "                          (default: 0 - no reconnecting)\n"
           , exe);
    exit(1);
}

void on_status(otp_node& a_node, const otp_connection* a_con,
        report_level a_level, const std::string& s)
{
    static const char* s_levels[] = {"INFO   ", "WARNING", "ERROR  "};
    std::cerr << s_levels[a_level] << "| " << s << std::endl;
}

otp_mailbox *g_io_server, *g_main;
static atom g_rem_node;

static const atom S  = atom("S");
static const atom N1 = atom("N1");
static const atom N2 = atom("N2");
static const atom N3 = atom("N3");

bool on_io_request(otp_mailbox& a_mbox, eixx::transport_msg*& a_msg, const boost::system::error_code& ec) {

    static const eterm s_put_chars = eterm::format("{io_request,_,_,{put_chars,S}}");

    varbind l_binding;
    if (s_put_chars.match(a_msg->msg(), &l_binding))
        std::cerr << "I/O request from server: "
                    << l_binding[S]->to_string() << std::endl;
    else
        std::cerr << "I/O server got a message: " << a_msg->msg() << std::endl;

    return true;
}

void on_main_msg(otp_mailbox& a_mbox, eixx::transport_msg*& a_msg, const boost::system::error_code& ec) {
    static const eterm s_now_pattern  = eterm::format("{rex, {N1, N2, N3}}");
    static const eterm s_stop         = atom("stop");

    varbind l_binding;

    if (a_msg) {
        const eterm& l_msg = a_msg->msg();

        if (l_msg.match(s_now_pattern, &l_binding)) {
            struct timeval tv =
                { l_binding[N1]->to_long() * 1000000 +
                    l_binding[N2]->to_long(),
                    l_binding[N3]->to_long() };
            struct tm tm;
            localtime_r(&tv.tv_sec, &tm);
            printf("Server time: %02d:%02d:%02d.%06ld\n",
                tm.tm_hour, tm.tm_min, tm.tm_sec, tv.tv_usec);
        } else if (l_msg.match(s_stop)) {
            a_mbox.node().stop();
            return;
        } else
            std::cout << "Unhandled message: " << l_msg << std::endl;
    } else if (ec) {
        // Make sure that remote node has a process registered as "test".
        // Try sending a message to it.
        static uint32_t n;
        if (n++ & 1)
            g_main->send_rpc(g_rem_node, "erlang", "now", list::make());
        else 
            g_io_server->send_rpc_cast(g_rem_node, atom("io"), atom("put_chars"),
                list::make("This is a test string"), &g_io_server->self());
        return;
    }

    a_mbox.async_receive(&on_main_msg, std::chrono::seconds(5));
}

void on_connect(otp_connection* a_con, const std::string& a_error) {
    if (!a_error.empty()) {
        std::cout << a_error << std::endl;
        return;
    }

    // Make sure that remote node has a process registered as "test".
    // Try sending a message to it.
    g_main->send_rpc(a_con->remote_nodename(), "erlang", "now", list::make());

    // Send an rpc request to print a string. The remote 
    g_io_server->send_rpc_cast(a_con->remote_nodename(), atom("io"), atom("put_chars"),
        list::make("This is a test string"), &g_io_server->self());
}

void on_disconnect(
    otp_node& a_node, const otp_connection& a_con,
    atom a_remote_node, const boost::system::error_code& err)
{
    std::cout << "Disconnected from remote node " << a_remote_node << std::endl;
    if (a_con.reconnect_timeout() == 0)
        a_node.stop();
}

int main(int argc, char* argv[]) {
    if (argc < 2)
        usage(argv[0]);

    const char* l_nodename = NULL, *l_remote = NULL, *use_cookie = "";
    connect::verbose_type verbose = connect::verboseness::level();
    int reconnect_secs = 0;

    for (int i = 1; i < argc && argv[i][0] == '-'; i++) {
        if (strcmp(argv[i], "-n") == 0 && i < argc-1)
            l_nodename = argv[++i];
        else if (strcmp(argv[i], "-r") == 0 && i < argc-1)
            l_remote = argv[++i];
        else if (strcmp(argv[i], "-c") == 0 && i < argc-1)
            use_cookie = argv[++i];
        else if (strcmp(argv[i], "-v") == 0 && i < argc-1)
            verbose = connect::verboseness::parse(argv[++i]);
        else if (strcmp(argv[i], "-t") == 0 && i < argc-1)
            reconnect_secs = atoi(argv[++i]);
        else
            usage(argv[0]);
    }

    if (!l_nodename || !l_remote)
        usage(argv[0]);

    boost::asio::io_service io_service;
    otp_node l_node(io_service, l_nodename, use_cookie);
    l_node.verbose(verbose);
    l_node.on_status     = on_status;
    l_node.on_disconnect = on_disconnect;

    g_io_server = l_node.create_mailbox("io_server");
    g_main      = l_node.create_mailbox("main");
    g_rem_node  = atom(l_remote);

    l_node.connect(&on_connect, g_rem_node, reconnect_secs);

    //otp_connection::connection_type* l_transport = a_con->transport();
    g_io_server->async_receive(&on_io_request, std::chrono::milliseconds(-1), -1);

    //l_node->send_rpc(self, a_con->remote_node(), atom("shell_default"), atom("ls"),
    //    list::make(), &io_server);
    g_main->async_receive(&on_main_msg, std::chrono::seconds(5));

    l_node.run();

    return 0;
}
