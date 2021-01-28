#include <reg51.h>       
#include "stdio.h"
#include "StateMachine.h"
#include <intrins.h>
#include <math.h>

#define uchar unsigned char
#define uint unsigned int
sbit EN = P2 ^7;  
sbit RS = P2 ^6;  
sbit RW = P2 ^5;  

sbit K1 = P1 ^0;
sbit K2 = P1 ^1;     
sbit K3 = P1 ^2;
sbit K4 = P1 ^3;
sbit K5 = P1 ^4;
sbit K6 = P1 ^5;
sbit K7 = P1 ^6;
sbit K8 = P1 ^7; 

uchar KEY_CODE[] = {
    0xee, 0xde, 0xbe, 0x7e,
    0xed, 0xdd, 0xbd, 0x7d,
    0xeb, 0xdb, 0xbb, 0x7b,
    0xe7, 0xd7, 0xb7, 0x77
};

uchar*ops = "123+456-789*.0=/";
double tmp;

/**
1 2 3 +
4 5 6 -
7 8 9 *
. 0 = /

( ) ~ c
*/

StateMachine machine;  // * 状态机

void scanf_matrix_keyboard(StateMachine_t machine, uchar *var);
void delay5MS();
void delay100MS();
void writeCMD(uchar com);
void showData(uchar dat);
void init();
void clear();


void main() {
    uchar num;
    init();    // * 初始化
    while (1) {
        scanf_matrix_keyboard(&machine, &num); // * 读入字符 | 重置状态机
        if (num == 0xff) { // * 强制刷新
            resetStateMachine(&machine);
            clear();
            continue;
        }
        stateMachineDoAction(&machine, num); // * 状态机执行动作
    }
}


/**
 * * 状态输出
 */
void stateToOutput(const State_t state) {
    char buf[MAX_QUEUE_LENGTH * 2];
	uchar m=0;
    sprintf(buf, "%g", state->STACK[state->stackTail].operand[0]);
    while (buf[m++]);
    writeCMD(0x4f + 0x80);
    writeCMD(0x04);
    --m;
    while (m--) showData(buf[m]);
}

/**
 * ! 提示错误 
 */
void stateShowError() {
    char*err="ERROR";
    clear();
    delay5MS();
    while (*err) showData(*err++);
}


/**
 * * 将队列中的数字写入到数字栈中
 * @param state 状态指针
 * @param locationOperand 数字栈的位置
 */ 
void statePushOperArray(const State_t state, int locationOperand) {
    if (state->bef != ')') {
        uchar x;
        double total = 0;
        sscanf(state->QUEUE + (state->QUEUE[0] == '~'?1:0), "%lf", &total);
        state->STACK[state->stackTail].operand[locationOperand] = 
            state->QUEUE[0] == '~'? sqrt(total): total;
        for (x = 0; x < state->queueTail; ++x) state->QUEUE[x] = 0;
        state->queueTail = 0;
    }
}


/**
 * * operand[0] <operator1> operand[1] -> operand[0]
 * @param state 状态指针
 */ 
int stateFirstCalculate(State_t state) {
    double ans, 
           num1 = state->STACK[state->stackTail].operand[0],
           num2 = state->STACK[state->stackTail].operand[1];
    uchar op = state->STACK[state->stackTail].operator1;
    switch (op) {
        case '+':
            ans = num1 + num2;
            break;
        case '-':
            ans = num1 - num2;
            break;
        case '*':
            ans = num1 * num2;
            break;
        case '/':
            if (-1e-5 < num2 && num2 < 1e-5) return 0;
            else ans = num1 / num2;
            break;
    }
    state->STACK[state->stackTail].operand[0] = ans;
    return 1;
}


/**
 * * operand[1] <operator2> operand[2] -> operand[1]
 * @param state 状态指针
 */ 
int stateLastCalculate(State_t state) {
    double ans, 
           num1 = state->STACK[state->stackTail].operand[1],
           num2 = state->STACK[state->stackTail].operand[2];
    uchar op = state->STACK[state->stackTail].operator2;
    switch (op) {
        case '+':
            ans = num1 + num2;
            break;
        case '-':
            ans = num1 - num2;
            break;
        case '*':
            ans = num1 * num2;
            break;
        case '/':
            if (-1e-5 < num2 && num2 < 1e-5) return 0;
            else ans = num1 / num2;
            break;
    }
    state->STACK[state->stackTail].operand[1] = ans;
    return 1;
}

/**
 * * 状态动作函数1 [初始状态]
 * @param state 状态指针
 * @param condition 新的字符
 */ 
int action1(State_t state, unsigned char condition) {
    clear();
    if (condition >= '0' && condition <= '9' && state->queueTail < MAX_QUEUE_LENGTH) {
        state->QUEUE[state->queueTail++] = condition;
        showData(condition);
        return 2;
    } else if (condition == '(') {
        ++state->stackTail;
        resetStackNode(state->STACK + state->stackTail);
        showData(condition);
        return 9;
    } else if (condition == '~') {
        state->QUEUE[state->queueTail++] = condition;
        showData(condition);
        return 9;
    } else if (condition == '.') {
        state->QUEUE[state->queueTail++] = condition;
        showData(condition);
        return 2;
    } else return -1;
}

/**
 * * 状态动作函数2 [数字]
 * @param state 状态指针
 * @param condition 新的字符
 */ 
int action2(State_t state, unsigned char condition) {
    if (condition >= '0' && condition <= '9' && state->queueTail < MAX_QUEUE_LENGTH) {
        state->QUEUE[state->queueTail++] = condition;
        showData(condition);
        return 2;
    } else if (condition == '+' || condition == '-' || condition == '*' || condition == '/') {
        statePushOperArray(state, 0);
        state->STACK[state->stackTail].operator1 = condition;
        showData(condition);
        return condition == '+' || condition == '-'? 3: condition == '*'? 4 : 5;
    } else if (condition == '.') {
        state->QUEUE[state->queueTail++] = condition;
        showData(condition);
        return 2;
    } else if (condition == '=') {
        statePushOperArray(state, 0);
        showData(condition);
        stateToOutput(state);
        return 1;
    } else return -1;
}

/**
 * 检查val == aim
 * @param val 被监测的值
 * @param aim 目标值
 */ 
int doubleCheck(double val, double aim) {
    return aim - 1e-5 < val && val < aim + 1e-5? 1: 0;
}

/**
 * 把当前栈状态传递给下一层，尽量保证位置正确
 * @param state 状态指针
 */ 
void popStack(State_t state) {
    if (!doubleCheck(state->STACK[state->stackTail-1].operand[1], 0) && 
        doubleCheck(state->STACK[state->stackTail-1].operand[2], 1)) {
        if (state->STACK[state->stackTail-1].operator2 == '*')
            state->STACK[state->stackTail-1].operand[2] = state->STACK[state->stackTail].operand[0];
        else {
            tmp = state->STACK[state->stackTail-1].operand[1];
            state->STACK[state->stackTail-1].operand[1] = state->STACK[state->stackTail].operand[0];
            state->STACK[state->stackTail-1].operand[2] = tmp;
        }
    } else state->STACK[state->stackTail-1].operand[1] = state->STACK[state->stackTail].operand[0];
    --state->stackTail;
}

/**
 * * 状态动作函数3 [+ -]
 * @param state 状态指针
 * @param condition 新的字符
 */ 
int action3(State_t state, unsigned char condition) {
    if (condition >= '0' && condition <= '9' && state->queueTail < MAX_QUEUE_LENGTH) {
        if (state->bef == ')') return -1;
        state->QUEUE[state->queueTail++] = condition;
        showData(condition);
        return 3;
    } else if (condition == '+' || condition == '-' || condition == '*' || condition == '/') {
        if (state->bef == '+' || state->bef == '-' || state->bef == '*' || state->bef == '/') return -1;
        statePushOperArray(state, 1);
        if ((condition == '+' || condition == '-') && !stateFirstCalculate(state)) return -1;
        if(condition == '+' || condition == '-') state->STACK[state->stackTail].operator1 = condition;
        else state->STACK[state->stackTail].operator2 = condition;
        showData(condition);
        return condition == '+' || condition == '-'? 3: condition == '*'? 6 : 7;
    } else if (condition == '.' || condition == '~') {
        if (state->QUEUE[0] == condition) return -1;
        state->QUEUE[state->queueTail++] = condition;
        showData(condition);
        return 3;
    } else if (condition == '=') {
        statePushOperArray(state, 1);
        showData(condition);
        if ((!doubleCheck(state->STACK[state->stackTail-1].operand[2], 1) || state->lastIndex == 6 || state->lastIndex == 7) && !stateLastCalculate(state)) return -1;
        if (!stateFirstCalculate(state)) return -1;
        stateToOutput(state);
        return 1;
    } else if (condition == '(') {
        ++state->stackTail;
        resetStackNode(state->STACK + state->stackTail);
        showData(condition);
        return 9;
    } else if (condition == ')') {
        statePushOperArray(state, 1);
        if(!stateFirstCalculate(state)) return -1;
        popStack(state);
        showData(condition);
        return 3;
    } else  return -1;
}

/**
 * * 状态动作函数4 [*]
 * @param state 状态指针
 * @param condition 新的字符
 */ 
int action4(State_t state, unsigned char condition) {
    if (condition >= '0' && condition <= '9' && state->queueTail < MAX_QUEUE_LENGTH) {
        if (state->bef == ')') return -1;
        state->QUEUE[state->queueTail++] = condition;
        showData(condition);
        return 4;
    } else if (condition == '+' || condition == '-' || condition == '*' || condition == '/') {
        if (state->bef == '+' || state->bef == '-' || state->bef == '*' || state->bef == '/') return -1;
        statePushOperArray(state, 1);
        if(!stateFirstCalculate(state)) return -1;
        state->STACK[state->stackTail].operator1 = condition;
        showData(condition);
        return condition == '+' || condition == '-'? 3: condition == '*'? 4 : 5;
    } else if (condition == '.' || condition == '~') {
        if (state->QUEUE[0] == condition) return -1;
        state->QUEUE[state->queueTail++] = condition;
        return 4;
    } else if (condition == '=') {
        statePushOperArray(state, 1);
        showData(condition);
        if (!stateFirstCalculate(state)) return -1;
        stateToOutput(state);
        return 1;
    } else if (condition == '(') {
        ++state->stackTail;
        resetStackNode(state->STACK + state->stackTail);
        showData(condition);
        return 9;
    } else if (condition == ')') {
        statePushOperArray(state, 1);
        if (!stateFirstCalculate(state)) return -1;
        popStack(state);
        showData(condition);
        return 3;
    } else return -1;
}

/**
 * * 状态动作函数5 [/]
 * @param state 状态指针
 * @param condition 新的字符
 */ 
int action5(State_t state, unsigned char condition) {
    if (condition >= '0' && condition <= '9' && state->queueTail < MAX_QUEUE_LENGTH) {
        if (state->bef == ')') return -1;
        state->QUEUE[state->queueTail++] = condition;
        showData(condition);
        return 5;
    } else if (condition == '+' || condition == '-' || condition == '*' || condition == '/') {
        if (state->bef == '+' || state->bef == '-' || state->bef == '*' || state->bef == '/') return -1;
        statePushOperArray(state, 1);
        if(!stateFirstCalculate(state)) return -1;
        state->STACK[state->stackTail].operator1 = condition;
        showData(condition);
        return condition == '+' || condition == '-'? 3: condition == '*'? 4 : 5;
    } else if (condition == '.' || condition == '~') {
        if (state->QUEUE[0] == condition) return -1;
        state->QUEUE[state->queueTail++] = condition;
        return 5;
    } else if (condition == '=') {
        statePushOperArray(state, 1);
        showData(condition);
        if (!stateFirstCalculate(state)) return -1;
        stateToOutput(state);
        return 1;
    } else if (condition == '(') {
        ++state->stackTail;
        resetStackNode(state->STACK + state->stackTail);
        showData(condition);
        return 9;
    } else if (condition == ')') {
        statePushOperArray(state, 1);
        if (!stateFirstCalculate(state)) return -1;
        popStack(state);
        showData(condition);
        return 3;
    } else return -1;
}

/**
 * * 状态动作函数6 [*]
 * @param state 状态指针
 * @param condition 新的字符
 */ 
int action6(State_t state, unsigned char condition) {
    if (condition >= '0' && condition <= '9' && state->queueTail < MAX_QUEUE_LENGTH) {
        if (state->bef == ')') return -1;
        state->QUEUE[state->queueTail++] = condition;
        showData(condition);
        return 6;
    } else if (condition == '+' || condition == '-' || condition == '*' || condition == '/') {
        if (state->bef == '+' || state->bef == '-' || state->bef == '*' || state->bef == '/') return -1;
        statePushOperArray(state, 2);
        if (!stateLastCalculate(state)) return -1;
        if ((condition=='+' || condition == '-') && !stateFirstCalculate(state)) return -1;
        if (condition == '+' || condition == '-') state->STACK[state->stackTail].operator1 = condition;
        else state->STACK[state->stackTail].operator2 = condition;
        showData(condition);
        return condition == '+' || condition == '-'? 3: condition == '*'? 6: 7;
    } else if (condition == '.' || condition == '~') {
        if (state->QUEUE[0] == condition) return -1;
        state->QUEUE[state->queueTail++] = condition;
        showData(condition);
        return 6;
    } else if (condition == '=') {
        statePushOperArray(state, 2);
        showData(condition);
        if (!stateLastCalculate(state)) return -1;
        if (!stateFirstCalculate(state)) return -1;
        stateToOutput(state);
        return 1;
    } else if (condition == '(') {
        ++state->stackTail;
        resetStackNode(state->STACK + state->stackTail);
        showData(condition);
        return 9;
    } else if (condition == ')') {
        statePushOperArray(state, 2);
        if (!stateLastCalculate(state)) return -1;
        if (!stateFirstCalculate(state)) return -1;
        popStack(state);
        showData(condition);
        return 3;
    } else return -1;
}

/**
 * * 状态动作函数7 [/]
 * @param state 状态指针
 * @param condition 新的字符
 */ 
int action7(State_t state, unsigned char condition) {
    if (condition >= '0' && condition <= '9' && state->queueTail < MAX_QUEUE_LENGTH) {
        if (state->bef == ')') return -1;
        state->QUEUE[state->queueTail++] = condition;
        showData(condition);
        return 7;
    } else if (condition == '+' || condition == '-' || condition == '*' || condition == '/') {
        if (state->bef == '+' || state->bef == '-' || state->bef == '*' || state->bef == '/') return -1;
        statePushOperArray(state, 2);
        if (!stateLastCalculate(state)) return -1;
        if ((condition=='+' || condition == '-') && !stateFirstCalculate(state)) return -1;
        if (condition == '+' || condition == '-') state->STACK[state->stackTail].operator1 = condition;
        else state->STACK[state->stackTail].operator2 = condition;
        showData(condition);
        return condition == '+' || condition == '-'? 3: condition == '*'? 6: 7;
    } else if (condition == '.' || condition == '~') {
        if (state->QUEUE[0] == condition) return -1;
        state->QUEUE[state->queueTail++] = condition;
        showData(condition);
        return 7;
    } else if (condition == '=') {
        statePushOperArray(state, 2);
        showData(condition);
        if (!stateLastCalculate(state)) return -1;
        if (!stateFirstCalculate(state)) return -1;
        stateToOutput(state);
        return 1;
    } else if (condition == '(') {
        ++state->stackTail;
        resetStackNode(state->STACK + state->stackTail);
        showData(condition);
        return 9;
    } else if (condition == ')') {
        statePushOperArray(state, 2);
        if (!stateLastCalculate(state)) return -1;
        if (!stateFirstCalculate(state)) return -1;
        popStack(state);
        showData(condition);
        return 3;
    } else return -1;
}

/**
 * * 状态动作函数9 [(]
 * @param state 状态指针
 * @param condition 新的字符
 */ 
int action9(State_t state, unsigned char condition) {
    if (condition >= '0' && condition <= '9' && state->queueTail < MAX_QUEUE_LENGTH) {
        if (state->bef == ')') return -1;
        state->QUEUE[state->queueTail++] = condition;
        showData(condition);
        return 2;
    } else if (condition == '(') {
        ++state->stackTail;
        state->STACK[state->stackTail] = state->STACK[state->stackTail-1];
        showData(condition);
        return 9;
    } else if (condition == '.' || condition == '~') {
        if (state->QUEUE[0] == condition) return -1;
        state->QUEUE[state->queueTail++] = condition;
        showData(condition);
        return 9;
    }
    else return -1;
}

/**
 * * 扫描独立键盘K值
 */ 
int scanfK() {
	if (!K1) {
        delay100MS();
        while (!K1);
        return 1;
    } else if (!K2) {
        delay100MS();
        while (!K2);
        return 2;
    } else if (!K3) {
        delay100MS();
        while (!K3);
        return 3;
    } else if (!K4) {
        delay100MS();
        while (!K4);
        return 4;
    } else return 0;
}

/**
 * * 扫描键盘并读取字符
 * @param machine 状态机指针
 * @param var 字符指针
 */ 
void scanf_matrix_keyboard(StateMachine_t machine, uchar*var) {
    uchar temp;
    int q = 1, K_res;
    temp = q;
    q = 0;
    while (1) {
        P3 = 0xff;

        if ((K_res = scanfK()) && K_res) { // * 监测到独立键盘有按键
            *var = "()~\0"[K_res-1];
            if (!*var) {
                resetStateMachine(machine);
                *var = 0xff;
            }
            break;
        }

        P3 = 0x0f;
        if (P3 != 0x0f) { // * 矩阵键盘有按键
            delay100MS();
            temp = P3;
            P3 = 0xf0;
            if (P3 != 0xf0) {
                *var = temp | P3;
                for (q = 0; q < 16; q++) if (*var == KEY_CODE[q]) { 
                    *var = ops[q];
                    break;
                }
                break;
            }
        }
    }
}

/**
 * * 延时5ms
 */ 
void delay5MS() {
    int n = 3000;
    while (n--);
}

/**
 * * 延时100ms
 */ 
void delay100MS() {
    uint n = 8000;
    while (n--);
}

/**
 * * 写命令到LCD屏
 * @param com 命令字
 */ 
void writeCMD(uchar com) {
    P0 = com;
    RS = 0;
    RW = 0;
    delay5MS();
    EN = 1;
    delay5MS();
    EN = 0;
}

/**
 * * 写字符到LCD屏
 * @param dat 字符
 */ 
void showData(uchar dat) {
    // EN = 1;
    RW = 1;
    RS = 0;
    if (P0 == 0x11) {
        writeCMD(0xC0);
    }
    P0 = dat;
    RS = 1;
    RW = 0;
    EN = 1;
    delay5MS();
    EN = 0;
}

/**
 * * 初始化
 */ 
void init() {
    EN = 0;
    writeCMD(0x38);
    writeCMD(0x0e);
    writeCMD(0x06);
    writeCMD(0x01);
    writeCMD(0x80);
    initStateMachine(&machine, stateShowError);
    machine.actions[1] = action1;
    machine.actions[2] = action2;
    machine.actions[3] = action3;
    machine.actions[4] = action4;
    machine.actions[5] = action5;
    machine.actions[6] = action6;
    machine.actions[7] = action7;
    machine.actions[9] = action9;
}

/**
 * * 清屏
 */ 
void clear() {
    EN = 0;
    writeCMD(0x01);
}
