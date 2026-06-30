enum State {
    READY,
    RUNNING,
    FINISHED,
    ZOMBIE
}

struct Process {
    vaddr esp,
    uint32 eip,
    vaddr kstack,
    uint32 pid,
    State state
}

uint32 LPID = 0;

void newProcess(vaddr func) {
    Process p;
    p.pid = LPID;
    LPID = LPID + 1;
    p.state = State::READY;
    p.kstack = func;
}