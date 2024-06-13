
## Coding rules
1. Indent with tabs.
2. Use auto for variable declarations when possible.
3. Never put "using namespace std" in any file.
4. Prefix class member variables with m_, this way you know you are using a member or local variable.
5. All namespaces, classes, functions and variables should be lower_case.
6. All files within a sub-directory must declare everything within a namespace with the same name as the directory. For example src/file/tsv_file.h must declare everything within the namespace file::
7. Prefer smart pointers over regular pointers.
8. Prefer if statements over switch statements.

## Indentation examples

Indent with tabs!

### pointers
```c++
// * and & are glued to the variable
int *ptr = new int[100];
int *ptr2 = &addr;
```

### operators
```c++
// Spaces between binary operators
int a = 1 + 2;
int b = multiple * (add1 + add2);
a += b;

// Unary operators are glued to variable
int a = 1;
a++;
int b = -a;
```

### functions
```c++
// Spaces after comma
int add(int a, int b) {
    return a + b;
}

// Spaces after comma here too
add(123, 333);
```

### classes
```c++
template<typename data_record>
class index_builder {
    public:
        index_builder(const std::string &db_name, size_t id);
        int public_func();

    private:
        int m_member;
        int m_counter;

        int private_func();
};
```


### if
```c++
// Space between "if" and "("
// Space between ")" and "{"
if (something) {
    do_something();
} else if (something_else) {
    do_something_else();
} else {
    do_else();
}
```

### loops
```c++
// Prefer range based loops.
for (const auto &iter : m_map) {

}
// But if you need a standard loop indent it like this.
for (int i = 0; i < 100; i++) {

} 
```

### memory allocation
```c++
// Avoid new/delete, use smart pointers everywhere.
// If you just need a regular pointer to memory do this:
std::unique_ptr<char[]> allocator;
try {
    allocator = std::make_unique<char[]>(1000);
} catch (std::bad_alloc &error) {
    // Handle allocation error.
}

char *ptr = allocator.get();

// Use ptr as regular pointer to 1000 chars.
// ptr will be deleted automatically when allocator goes out of scope.
```



