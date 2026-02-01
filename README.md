You are building a library to manage “resources” in a multi-threaded application. Each resource is of a generic type T. Your manager should allow adding, removing, and incrementing resources.

Requirements:

Implement a template class ResourceManager<T> that can store multiple resources.

Increment resources using operator+= — only if T supports it.

Ensure thread safety when adding, removing, or incrementing resources.

Avoid memory leaks or dangling references.

Support deep copy and move semantics.

Provide a way to access a resource by index safely.