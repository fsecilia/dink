# dink
This is a simple library that does one very surprising thing: it composes an entire object graph automatically

Dink is a compile-time, header-only, c++20 dependency injection container. It manages both construction and lifetimes of objects in an object graph. The graph and the objects it contains are constructed automatically. Objects may be constructed by invoking their ctor, a factory function, or a factory instance. Parameters passed to construction are deduced automatically, then constructed recursively the same way as the requested type. Requests to resolve a reference or reference to const will reference a shared instance in the initialized data segment, transient instances are created on the fly otherwise.

Deduction uses implicit conversion templates to try and invoke a particular construction method, first with 0 parameters, then increasing the number of parameters until a match is found or an error limit is reached. 

That's it. The mechanism is basic, but it composes to achieve arbitrary complexity.

Given an arbitrary type `arbitrary_type_t`:
```  
    auto dink = dink{};
    auto result = dink.template resolve<arbitrary_type_t>();
```
Here, `result` is automatically constructed with literal type `arbitrary_type_t`. If the type has a static `construct` method, the method is used to construct the instance, otherwise the ctor is used. If the ctor or static construct method is overloaded, the overload with the smallest number of parameters is chosen. All parameters are recursively resolved first, then passed to the constructor or static construct method.

Customization points are supplied for more complex constructions, like types which require factories and when data from outside the graph is required. Factories are associated via specialization, and data is bound by the composer:
```
// specialize factory_t to construct arbitrary_factoried_type_t using instance of arbitrary_factory_t
struct factory_t<arbitrary_factoried_type_t>
    : factories::external_t<arbitrary_factoried_type_t, arbitrary_factory_t>
{
};

auto APIENTRY WinMain(HINSTANCE instance, HINSTANCE, PSTR, int) -> int
{
    auto dink = dink{};
    dink.bind(instance); // binds requests for an HINSTANCE to instance
    auto result = dink.template resolve<arbitrary_factoried_type_t>();
    return EXIT_SUCCESS;
}
```
Here, as dink instantiates parts of the graph, any time it encounters a ctor or factory argument of type HINSTANCE, it uses the bound value `instance` instead of default initializing.

This is original work based on a [talk](https://youtu.be/yVogS4NbL6U?si=nmCoA6SG797rT-4m) from [Kris Jusiak](linkedin.com/in/kris-jusiak). The sauce is using implicit conversion templates hooked up to a recursive composer to choose a ctor automatically.

v1.0 works against types, but not templates directly. This means you still have to instantiate all templates you intend to wire. Typically, this means there is a block of template instantiations near the composition, along with instantiations of their dependencies. This block can be avoided when dink learns how to resolve templates directly. That will be v2.0.
