# dink
This is a simple library that does one very surprising thing: it composes an entire object graph automatically

Dink is a compile-time, header-only, c++20 dependency injection framework. It manages both construction and lifetimes of objects in an object graph. The graph and the objects it contains are constructed automatically. Objects may be constructed by invoking their ctor, a factory function, or a factory instance. Parameters passed to construction are deduced automatically, then constructed recursively the same way as the requested type. Requests to resolve a reference or reference to const will reference a shared instance in the uninitialized data segment, transient instances are created on the fly otherwise.

Deduction uses implicit conversion templates to try and invoke a particular construction method, first with 0 parameters, then increasing the number of parameters until a match is found or an error limit is reached. 

That's it. The mechanism is basic, but it composes to achieve arbitrary complexity.

Given:
```  
    auto composer = dink::composer_t{};
    auto result = composer.template resolve<arbitrary_type_t>();  
```
`result` is automatically constructed with literal type `arbitrary_type_t`. If the constructor is overloaded, the overload with the smallest number of parameters is chosen. All parameters are recursively resolved first, then passed to the constructor.

This is original work based on a [talk](https://youtu.be/yVogS4NbL6U?si=nmCoA6SG797rT-4m) from [Kris Jusiak](linkedin.com/in/kris-jusiak). The sauce is using implicit conversion templates hooked up to a recursive composer to choose a ctor automatically.
