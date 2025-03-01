#ifndef GRAPH_ERROR_HPP
#define GRAPH_ERROR_HPP

#include <stdexcept>
#include <string>

namespace driver::error {

class GraphError : public std::runtime_error {
public:
    explicit GraphError(const std::string& message) : std::runtime_error(message) {}
    explicit GraphError(const char* message) : std::runtime_error(message) {}
};

class FileReadError : public GraphError {
public:
    explicit FileReadError(const std::string& filename) 
        : GraphError("Error reading file: " + filename) {}
};

class InvalidLineError : public GraphError {
public:
    explicit InvalidLineError(const std::string& line) 
        : GraphError("Invalid line: " + line) {}
};

class FunctionNotImplementedError : public GraphError {
public:
    explicit FunctionNotImplementedError(const std::string& function_name) 
        : GraphError("Function not implemented: " + function_name) {}

};

class GraphLogicalError : public GraphError {
public:
    explicit GraphLogicalError(const std::string& message) 
        : GraphError("Logical error: " + message) {}
};

class ReaderDoesNotSupportError : public GraphError {
public:
    explicit ReaderDoesNotSupportError(const std::string& reader_name) 
        : GraphError("Reader does not support: " + reader_name) {}
};


class VertexIndexOutOfBoundError : public GraphError {
public:
    explicit VertexIndexOutOfBoundError(const std::string& vertex_id) 
        : GraphError("Vertex out of bound " + vertex_id) {}
};

} // namespace driver::error

#endif // GRAPH_ERROR_HPP
