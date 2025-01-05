int i = 1.0f; // float to int works

float f = 1; // int to float works
int* pi = (int)1.0f; // float to int to int* works
float* pf = (float*)1.0f; // error: float to ptr type


struct A{
    int a;
};


int arr[1];  
int* p = arr; // arr to ptr works

const int arr2[1];
int* p2 = arr2; // warning: const to non consts

void* returnVoidPtr(){
    return (void*)1; // warning
}

void takesPtrPtr(int** ptr){
    
}

void takesConstPtr(const int* ptr){
    
}

void takesConstInt(const int n){
    
}

void takesPtr(int* ptr){
    
}

void takesInt(int n){
    
}


int foo(){
    float* pf = (void*)pi; // no warning
    pf = pi; // warning: int* to float*
    pf = (float*)pi; // warning: int* to float*
    pf = (void*)pi; // no warning
    void *a = pi; // no warning
    a = pi; // no warning
    int* b = a; // no warning : int* to int*
    int* c = returnVoidPtr(); // no warning
    
    int mi = 0;
    const int ci = 0;
    int* pmi;
    const int* pci;
    const int** ppci;
    takesConstInt(mi); // works: non const to const
    takesConstInt(ci); // works: const to const
    takesInt(mi); // works: non const int to non const int
    takesInt(ci); // works: const int to non const int (copy)

    takesConstPtr(pmi); // works: non const ptr to const ptr 
    takesConstPtr(pci); // works: const ptr to const ptr
    takesPtr(pmi); // works: non const ptr to non const ptr
    takesPtr(pci); // warning: const ptr to non const ptr

    takesPtrPtr(ppci); // warning: const int** to int**
    
    struct A strA = (struct A)1; // error: int to struct 
    struct A strB = strA;
    struct A strC = (struct A)strB; // works: struct to struct




    return 0;
}
