//----------------------------------------------------------------------------
/// \file  basic_otp_mailbox.hpp
//----------------------------------------------------------------------------
/// \brief A class implementing mailbox functionality.
///        The mailbox is a queue of messages identified by a pid.
///        It implements asynchronous reception of messages.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-20
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the eixx (Erlang C++ Interface) library.

Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

***** END LICENSE BLOCK *****
*/

#ifndef _EIXX_BASIC_OTP_MAILBOX_HPP_
#define _EIXX_BASIC_OTP_MAILBOX_HPP_

#include <boost/asio.hpp>
#include <eixx/util/async_wait_timeout.hpp>
#include <eixx/util/async_queue.hpp>
#include <eixx/marshal/eterm.hpp>
#include <eixx/connect/transport_msg.hpp>
#include <eixx/connect/verbose.hpp>
#include <chrono>
#include <list>
#include <set>

namespace EIXX_NAMESPACE {
namespace connect {

template<typename Alloc, typename Mutex> class basic_otp_node;

using EIXX_NAMESPACE::marshal::list;
using EIXX_NAMESPACE::marshal::epid;

/**
 * Provides a simple mechanism for exchanging messages with Erlang
 * processes or other instances of this class.
 *
 * Mailbox is the way to send an receive messages. Outgoing messages
 * will be forwarded to another local mailbox or a pid on another node.
 *
 * MailBox will receive and queue messages
 * delivered with the deliver() method. The messages can be
 * dequeued using the receive() family of functions.
 *
 * You can get the message in order of arrival, or use pattern matching
 * to explore the message queue.
 *
 * Each mailbox is associated with a unique {@link epid
 * pid} that contains information necessary for message delivery.
 *
 * Messages to remote nodes are encoded in the Erlang external binary
 * format for transmission.
 *
 * Mailboxes can be linked and monitored in much the same way as
 * Erlang processes. If a link is active when a mailbox is {@link
 * #close closed}, any linked Erlang processes or mailboxes will be
 * sent an exit signal.
 *
 * TODO: When retrieving messages from a mailbox that has received an exit
 * signal, an {@link OtpErlangExit OtpErlangExit} exception will be
 * raised. Note that the exception is queued in the mailbox along with
 * other messages, and will not be raised until it reaches the head of
 * the queue and is about to be retrieved.
 *
 */
template <typename Alloc, typename Mutex>
class basic_otp_mailbox
{
public:
    typedef std::shared_ptr<basic_otp_mailbox<Alloc, Mutex> > pointer;

    typedef std::function<
        void (basic_otp_mailbox<Alloc, Mutex>&,
              transport_msg<Alloc>*&,
              const boost::system::error_code&)
    > receive_handler_type;

    typedef util::async_queue<transport_msg<Alloc>*, Alloc> queue_type;

    template<typename A, typename M> friend class basic_otp_node;
    template<typename T, typename A> friend class util::async_queue;
    template<typename _R, typename... _ArgTypes> friend class std::function;
    template<typename _R, typename _F, typename... _Args> friend class std::_Function_handler;

    void name(const atom& a_name) { m_name = a_name; }

private:
    boost::asio::io_service&                m_io_service;
    basic_otp_node<Alloc, Mutex>&           m_node;
    epid<Alloc>                             m_self;
    atom                                    m_name;
    std::set<epid<Alloc> >                  m_links;
    std::map<ref<Alloc>, epid<Alloc> >      m_monitors;
    std::shared_ptr<queue_type>             m_queue;
    std::chrono::system_clock::time_point   m_time_freed;   // Cache time of this mbox
    receive_handler_type                    m_handler;      // Called on async_receive

    void do_deliver(transport_msg<Alloc>* a_msg);

    bool operator() (transport_msg<Alloc>*& a_msg, const boost::system::error_code& ec);

public:
    basic_otp_mailbox(
            basic_otp_node<Alloc, Mutex>& a_node, const epid<Alloc>& a_self,
            const atom& a_name = atom(), boost::asio::io_service* a_svc = NULL,
            const Alloc& a_alloc = Alloc())
        : basic_otp_mailbox(a_node, a_self, a_name, 255, a_svc, a_alloc)
    {}

    basic_otp_mailbox(
            basic_otp_node<Alloc, Mutex>& a_node, const epid<Alloc>& a_self,
            const atom& a_name = atom(), int a_queue_size = 255,
            boost::asio::io_service* a_svc = NULL, const Alloc& a_alloc = Alloc())
        : m_io_service(a_svc ? *a_svc : a_node.io_service())
        , m_node(a_node), m_self(a_self)
        , m_name(a_name)
        , m_queue(new queue_type(m_io_service, a_queue_size, a_alloc))
    {}

    ~basic_otp_mailbox() {
        close();
    }

    /// @param a_reg_remove when true the mailbox's pid is removed from registry.
    ///          Only pass false when invoking from the registry on destruction.
    void close(const eterm<Alloc>& a_reason = am_normal, bool a_reg_remove = true);

    basic_otp_node<Alloc, Mutex>&   node()          const { return m_node;       }
    /// Pid associated with this mailbox.
    const epid<Alloc>&              self()          const { return m_self;       }
    /// Registered name of this mailbox (if it was given a name).
    const atom&                     name()          const { return m_name;       }
    boost::asio::io_service&        io_service()    const { return m_io_service; }
    /// Queue of pending received messages.
    //queue_type&                     queue()               { return m_queue;      }
    /// Indicates if mailbox doesn't have any pending messages
    bool                            empty()         const { return m_queue.empty(); }

    /// Register current mailbox under the given name
    bool reg(const atom& a_name) { return m_node.register_mailbox(a_name, *this); }

    bool operator== (const basic_otp_mailbox& rhs) const { return self() == rhs.self(); }
    bool operator!= (const basic_otp_mailbox& rhs) const { return self() != rhs.self(); }

    /// Clear mailbox's queue of awaiting messages
    void clear() { m_queue->reset(); }

    /// Print pid and regname of the mailbox to the given stream
    std::ostream& dump(std::ostream& out) const;

    /// Find the first message in the mailbox matching a pattern.
    /// Return the message, and if \a a_binding is not NULL set the binding variables.
    /// The call is not thread-safe and should be evaluated in the thread running the
    /// mailbox node's service.
    transport_msg<Alloc>*
    match(const eterm<Alloc>& a_pattern, varbind* a_binding = NULL);

    /// Dequeue the next message from the mailbox.  The call is non-blocking and
    /// returns NULL if no messages are waiting.
    transport_msg<Alloc>* receive() {
        transport_msg<Alloc>* m;
        return m_queue->dequeue(m) ? m : nullptr;
    }

    /**
     * Call a handler on asynchronous delivery of message(s).
     *
     * The call is non-blocking. If returns Upon timeout
     * or delivery of a message to the mailbox the handler \a h will be
     * invoked.  The handler must have a signature with two arguments:
     * \verbatim
     * void handler(basic_otp_mailbox<Alloc, Mutex>& a_mailbox,
     *              transport_msg<Alloc>*& a_msg,
     *              ::boost::system::error_code& a_errc);
     * \endverbatim
     * In case of timeout the error will be set to non-zero value
     *
     * @param h is the handler to call upon arrival of a message
     * @param a_timeout is the timeout interval to wait for message (-1 = infinity)
     * @param a_repeat_count is the number of messages to wait (-1 = infinite)
     * @return true if the message was synchronously received
     **/
    bool async_receive(receive_handler_type h,
                       std::chrono::milliseconds a_timeout = std::chrono::milliseconds(-1),
                       int a_repeat_count = 0
                      )
        throw (std::runtime_error);

    /**
     * Cancel pending asynchronous receive operation
     */
    void cancel_async_receive() {
        m_handler = nullptr;
        m_queue->cancel();
    }

    /// Deliver a message to this mailbox. The call is thread-safe.
	void deliver(const transport_msg<Alloc>& a_msg) {
        transport_msg<Alloc>* p = new transport_msg<Alloc>(a_msg);
        m_queue->enqueue(p);
    }

    /// Deliver a message to this mailbox. The call is thread-safe.
    void deliver(transport_msg<Alloc>&& a_msg) {
        transport_msg<Alloc>* p = new transport_msg<Alloc>(std::move(a_msg));
        m_queue->enqueue(p);
    }

    /// Send a message \a a_msg to a pid \a a_to.
    void send(const epid<Alloc>& a_to, const eterm<Alloc>& a_msg) {
        m_node.send(self(), a_to, a_msg);
    }
    /// Send a message \a a_msg to the local process registered as \a a_to.
    void send(const atom& a_to, const eterm<Alloc>& a_msg) {
        m_node.send(self(), a_to, a_msg);
    }
    /// Send a message \a a_msg to the process registered as \a a_to on remote node \a a_node.
    void send(const atom& a_node, const atom& a_to, const eterm<Alloc>& a_msg) {
        m_node.send(self(), a_node, a_to, a_msg);
    }

    /**
     * Block until response for a RPC call arrives.
     * @return a pointer to ErlTerm containing the response
     * @exception EpiConnectionException if there was an connection error
	 * @throw EpiBadRPC if the corresponding RPC was incorrect
     */
    //bool receive_rpc_reply(const eterm<Alloc>& a_reply);

    /**
	 * Send an RPC request to a remote Erlang node.
     * @param node remote node where execute the funcion.
	 * @param mod the name of the Erlang module containing the
	 * function to be called.
	 * @param fun the name of the function to call.
	 * @param args a list of Erlang terms, to be used as arguments
	 * to the function.
     * @throws EpiBadArgument if function, module or nodename are too big
     * @throws EpiInvalidTerm if any of the args is invalid
	 * @throws EpiEncodeException if encoding fails
     * @throws EpiConnectionException if send fails
	 */
    void send_rpc(const atom& a_node,
                  const atom& a_mod,
                  const atom& a_fun,
                  const list<Alloc>& args,
                  const epid<Alloc>* gleader = NULL) {
        m_node.send_rpc(self(), a_node, a_mod, a_fun, args);
    }

    /// Execute an equivalent of rpc:cast(...). Doesn't return any value.
    void send_rpc_cast(const atom& a_node, const atom& a_mod,
            const atom& a_fun, const list<Alloc>& args,
            const epid<Alloc>* gleader = NULL)
            throw (err_bad_argument, err_no_process, err_connection) {
        m_node.send_rpc_cast(self(), a_node, a_mod, a_fun, args, gleader);
    }

    /// Send exit message to all linked pids and monitoring pids
    void break_links(const eterm<Alloc>& a_reason);

    /// Attempt to kill a remote process by sending it
    /// an exit message to a_pid, with reason \a a_reason
    void exit(const epid<Alloc>& a_pid, const eterm<Alloc>& a_reason) {
        m_node.send_exit(transport_msg<Alloc>::EXIT2, self(), a_pid, a_reason);
    }

    /// Link mailbox to the given pid.
    /// The given pid will receive an exit message when \a a_pid dies.
    /// @throws err_no_process
    /// @throws err_connection
    void link(const epid<Alloc>& a_to) throw (err_no_process, err_connection) {
        if (self() == a_to)
            return;
        if (m_links.find(a_to) != m_links.end())
            return;
        m_node.link(self(), a_to);
        m_links.insert(a_to);
    }

    /// UnLink the given pid
    void unlink(const epid<Alloc>& a_to) {
        if (m_links.erase(a_to) == 0)
            return;
        m_node.unlink(self(), a_to);
    }

    /// Set up a monitor of a remote \a a_target_pid.
    const ref<Alloc>& monitor(const epid<Alloc>& a_target_pid) {
        if (self() == a_target_pid)
            return;
        const ref<Alloc>& l_ref = m_node.send_monitor(self(), a_target_pid);
        m_monitors[l_ref] = a_target_pid;
    }

    /// Demonitor the pid monitored using \a a_ref reference.
    void demonitor(const ref<Alloc>& a_ref) {
        typename std::map<ref<Alloc>, epid<Alloc> >::iterator it = m_monitors.find(a_ref);
        if (it == m_monitors.end())
            return;
        m_node.send_demonitor(self(), it->second, a_ref);
        m_monitors.erase(it);
    }
};

//------------------------------------------------------------------------------
// basic_otp_mailbox implementation
//------------------------------------------------------------------------------

template <typename Alloc, typename Mutex>
void basic_otp_mailbox<Alloc, Mutex>::
close(const eterm<Alloc>& a_reason, bool a_reg_remove) {
    m_handler = nullptr;
    m_queue->reset();
    m_time_freed = std::chrono::system_clock::now();
    if (a_reg_remove)
        m_node.close_mailbox(this);
    break_links(a_reason);
    m_name = atom();
}

template <typename Alloc, typename Mutex>
bool basic_otp_mailbox<Alloc, Mutex>::
operator() (transport_msg<Alloc>*& a_msg, const boost::system::error_code& ec)
{
    if (m_time_freed.time_since_epoch().count() == 0 || !m_handler)
        return false;
    m_handler(*this, a_msg, ec);
    if (a_msg) {
        delete a_msg;
        a_msg = NULL;
    }
    return true;
}

template <typename Alloc, typename Mutex>
bool basic_otp_mailbox<Alloc, Mutex>::
async_receive(receive_handler_type h, std::chrono::milliseconds a_timeout,
              int a_repeat_count) throw (std::runtime_error)
{
    m_handler = h;
    return m_queue->async_dequeue(*this, a_timeout, a_repeat_count);
}

template <typename Alloc, typename Mutex>
void basic_otp_mailbox<Alloc, Mutex>::
break_links(const eterm<Alloc>& a_reason)
{
    for (typename std::set<epid<Alloc> >::const_iterator
            it=m_links.begin(), end = m_links.end(); it != end; ++it)
        try { m_node.send_exit(self(), *it, a_reason); } catch(...) {}
    for (typename std::map<ref<Alloc>, epid<Alloc> >::const_iterator
            it = m_monitors.begin(), end = m_monitors.end(); it != end; ++it)
        try { m_node.send_monitor_exit(self(), it->second, it->first, a_reason); } catch(...) {}
    if (!m_links.empty())    m_links.clear();
    if (!m_monitors.empty()) m_monitors.clear();
}

template <typename Alloc, typename Mutex>
transport_msg<Alloc>* basic_otp_mailbox<Alloc, Mutex>::
match(const eterm<Alloc>& a_pattern, varbind* a_binding)
{
    for (typename queue_type::iterator it = m_queue.begin(), e = m_queue.end();
            it != e; ++it)
    {
        transport_msg<Alloc>* p = *it;
        BOOST_ASSERT(p);
        if (a_pattern.match(p->msg(), a_binding)) {
            // Found a match
            m_queue.erase(it);
            return p;
        }
    }
    return NULL;
}

template <typename Alloc, typename Mutex>
void basic_otp_mailbox<Alloc, Mutex>::
do_deliver(transport_msg<Alloc>* a_msg)
{
    try {
        switch (a_msg->type()) {
            case transport_msg<Alloc>::LINK:
                BOOST_ASSERT(a_msg->recipient_pid() == self());
                m_links.insert(a_msg->sender_pid());
                delete a_msg;
                return;

            case transport_msg<Alloc>::UNLINK:
                BOOST_ASSERT(a_msg->recipient_pid() == self());
                m_links.erase(a_msg->sender_pid());
                delete a_msg;
                return;

            case transport_msg<Alloc>::MONITOR_P:
                BOOST_ASSERT((a_msg->recipient().type() == PID && a_msg->recipient_pid() == self())
                           || a_msg->recipient().to_atom() == m_name);
                m_monitors.insert(
                    std::pair<ref<Alloc>, epid<Alloc> >(a_msg->get_ref(), a_msg->sender_pid()));
                delete a_msg;
                return;

            case transport_msg<Alloc>::DEMONITOR_P:
                m_monitors.erase(a_msg->get_ref());
                delete a_msg;
                return;

            case transport_msg<Alloc>::MONITOR_P_EXIT:
                m_monitors.erase(a_msg->get_ref());
                m_queue.push_back(a_msg);
                break;

            case transport_msg<Alloc>::EXIT2:
            case transport_msg<Alloc>::EXIT2_TT:
                BOOST_ASSERT(a_msg->recipient_pid() == self());
                m_links.erase(a_msg->sender_pid());
                m_queue.push_back(a_msg);
                break;

            default:
                m_queue.push_back(a_msg);
        }
    } catch (std::exception& e) {
        a_msg->set_error_flag();
        m_queue.push_back(a_msg);
    }
}

template <typename Alloc, typename Mutex>
std::ostream& basic_otp_mailbox<Alloc, Mutex>::
dump(std::ostream& out) const {
    out << "#Mbox{pid=" << self();
    if (m_name != atom()) out << ", name=" << m_name;
    return out << '}';
}

} // namespace connect
} // namespace EIXX_NAMESPACE

namespace std {

    template <typename Alloc, typename Mutex>
    ostream& operator<< (ostream& out,
        const EIXX_NAMESPACE::connect::basic_otp_mailbox<Alloc,Mutex>& a_mbox)
    {
        return a_mbox.dump(out);
    }

} // namespace std

#endif // _EIXX_BASIC_OTP_MAILBOX_HPP_
