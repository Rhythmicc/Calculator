#define MAX_STATE_LENGTH 10
#define MAX_STACK_LENGTH 10
#define MAX_QUEUE_LENGTH 16

typedef struct {
    double operand[3];
    unsigned char operator1, operator2;
} StackNode;

void set0StackNode(StackNode*node) {
    node->operand[0] = node->operand[1] = 0;
    node->operand[2] = 1;
    node->operator1 = 0; 
    node->operator2 = 0;
}
    
void resetStackNode(StackNode*node) {
    set0StackNode(node);
    node->operator1 = '+';
    node->operator2 = '*';
}

typedef struct State{
    StackNode STACK[MAX_STACK_LENGTH];
    unsigned char QUEUE[MAX_QUEUE_LENGTH], _cur, bef;
    int stackTail, queueTail, index, lastIndex;

    void (*error)(); // 错误提醒
} State, *State_t;

typedef struct {
    State cursor;  // 当前状态
    int (*actions[MAX_STATE_LENGTH])(State_t,unsigned char); // 转移函数
} StateMachine, *StateMachine_t;

void resetStateMachine(StateMachine_t machine) {
    unsigned char x;
    machine->cursor.lastIndex = -1;
    machine->cursor.index = 1;
    machine->cursor.stackTail = 0;
    machine->cursor.queueTail = 0;
    machine->cursor._cur = machine->cursor.bef = 0xff;
    for (x = 1; x < MAX_STACK_LENGTH; ++x) set0StackNode(machine->cursor.STACK + x);
    resetStackNode(machine->cursor.STACK);
    for (x = 0; x < MAX_QUEUE_LENGTH; ++x) machine->cursor.QUEUE[x] = 0;
}

void initStateMachine(StateMachine_t machine, void (*error)()) {
    machine->cursor.error = error;
    resetStateMachine(machine);
}

void stateMachineDoAction(StateMachine_t machine, unsigned char condition) {
    int next_state; 
    
    machine->cursor.bef = machine->cursor._cur;
    machine->cursor._cur = condition;
    
    next_state = machine->actions[machine->cursor.index](&(machine->cursor), condition);
    if (next_state > 0) {
        machine->cursor.lastIndex = machine->cursor.index;
        machine->cursor.index = next_state;
        if (next_state == 1) resetStateMachine(machine);
    } else {
        machine->cursor.error();
        resetStateMachine(machine);
    } 
}
