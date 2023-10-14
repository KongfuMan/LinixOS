struct spinlock{
    uint locked;
    struct cpu* cpu;
    char* name;
};