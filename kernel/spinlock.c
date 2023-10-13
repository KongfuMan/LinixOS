void acquire(struct lock *lk){
    push_off();
    if (!holding(lk)){
        panic("acquire: already held the lock");
    }

    while(__sync_lock_test_and_set(&(lk->locked), 1) == 1){}

    __sync_synchronize();
}

void release(struct lock *lk){
    if (!holding(lk)){
        panic("release: not hold lock");
    }

    lk->cpu = 0;

    __sync_synchronize();

     __sync_lock_release(&lk->locked);
     
    pop_off();
}

void push_off(){
    int old = intr_get(); //get current interrupt status
    intr_off();
    struct cpu *curr_cpu = current_cpu();
    if (curr_cpu->off == 0){
        curr_cpu->intena = old;
    }
    
    curr_cpu->off += 1;
}

void pop_off(){
    if (intr_get()){
        panic("pop_off - interruptible");
    }

    struct cpu *curr_cpu = current_cpu();

    if (curr_cpu->off < 1){
        panic("pop_off");
    }

    curr_cpu->off -= 1;
    if (curr_cpu->off == 0 && curr_cpu->intena){
        intr_on();
    }
}

int holding(struct lock *lk){
    if (lk->locked && lk->cpu == current_cpu()){
        return 1;
    }
    return 0;
}