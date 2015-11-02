/*
 * Normally this won't work. This is called 'forward declaration', with Bar
 * being an 'incomplete type', where we can only define pointer/reference
 * to it, or declar, but not define, functions that uses such incomplete
 * type as parameter or return type.
 *
 * Why this works?
 *
 * Notice this is Foo::Bar, not Bar. Foo::Bar is part of the declartion of
 * Foo!
 * I guess it's because the compiler will prcess classes in two steps: the
 * member declarations are compiled first, after which the member function
 * bodies are processes. See C++ Primer (7.1.2, P338 or 7.4.1 P, P368).
 * If instead, we're going to move struct Bar outside of struct Foo, then
 * compile will fail at Bar Foo::getBar() const because we provides the 
 * definition of the function.
 */
struct Foo
{
    struct Bar;
    
    Bar getBar() const
    {
        return Bar(1);
    }

    struct Bar
    {
        Bar(int i) : _i(i) { }
    private:
        int _i;
    };
};

int main()
{
    return 0;
}
