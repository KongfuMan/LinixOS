struct lock{
    int locked;
    struct cpu cpu;
    char* name;
}