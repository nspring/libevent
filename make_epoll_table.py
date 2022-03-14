#!/usr/bin/python2

def get(old,wc,rc,cc,hc,ec):
    if ('xxx' in (rc, wc, cc,hc,ec)):
        return "0",255

    if ('add' in (rc, wc, cc,hc,ec)):
        events = []
        if rc == 'add' or (rc != 'del' and 'r' in old):
            events.append("EPOLLIN")
        if wc == 'add' or (wc != 'del' and 'w' in old):
            events.append("EPOLLOUT")
        if cc == 'add' or (cc != 'del' and 'c' in old):
            events.append("EPOLLRDHUP")
        if hc == 'add' or (cc != 'del' and 'c' in old):
            events.append("EPOLLHUP")
        if ec == 'add' or (cc != 'del' and 'c' in old):
            events.append("EPOLLERR")

        if old == "0":
            op = "EPOLL_CTL_ADD"
        else:
            op = "EPOLL_CTL_MOD"
        return "|".join(events), op

    if ('del' in (rc, wc, cc)):
        delevents = []
        modevents = []
        op = "EPOLL_CTL_DEL"

        if 'r' in old:
            modevents.append("EPOLLIN")
        if 'w' in old:
            modevents.append("EPOLLOUT")
        if 'c' in old:
            modevents.append("EPOLLRDHUP")
        if 'h' in old:
            modevents.append("EPOLLHUP")
        if 'e' in old:
            modevents.append("EPOLLERR")

        for item, event in [(rc,"EPOLLIN"),
                            (wc,"EPOLLOUT"),
                            (cc,"EPOLLRDHUP"),
                            (hc,"EPOLLHUP"),
                            (ec,"EPOLLERR"), ]:
            if item == 'del':
                delevents.append(event)
                if event in modevents:
                    modevents.remove(event)

        if modevents:
            return "|".join(modevents), "EPOLL_CTL_MOD"
        else:
            return "|".join(delevents), "EPOLL_CTL_DEL"

    return 0, 0


def fmt(op, ev, old, wc, rc, cc, hc, ec):
    entry = "{ %s, %s },"%(op, ev)
    print "\t/* old=%3s, write:%3s, read:%3s, close:%3s, hup:%3s, err:%3s  */\n\t%s" % (
        old, wc, rc, cc, hc, ec, entry)
    return len(entry)


def state_from_index(i):
    ret = ''
    if (i & 16):
        ret += 'e'
    if (i & 8):
        ret += 'h'
    if (i & 4):
        ret += 'c'
    if (i & 2):
        ret += 'r'
    if (i & 1):
        ret += 'w'
    if (ret == ''):
        ret = '0'
    return(ret)

old_options = [state_from_index(i) for i in range(32)]; 
no_add_del_inval = ('0', 'add', 'del', 'xxx')

print "\t/* begin epoll_op_table */";
for old in old_options:
    for ec in no_add_del_inval:
        for hc in no_add_del_inval:
            for wc in no_add_del_inval:
                for rc in no_add_del_inval:
                    for cc in no_add_del_inval:

                        op,ev = get(old,wc,rc,cc,hc,ec)

                        fmt(op, ev, old, wc, rc, cc, hc, ec)

print "\t/* end epoll_op_table */";
