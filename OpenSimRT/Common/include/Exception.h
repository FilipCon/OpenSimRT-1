#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <string>
#include <sstream>

/**
 * \brief An exception that prints the file and line number along with
 * the message.
 */
class FileLineException : public std::exception {
 public:
    FileLineException(const std::string &arg, const char* file, int line)
        : std::exception() {
        std::ostringstream o;
        o << file << ":" << line << ": " << arg;
        msg = o.str();
    }
    ~FileLineException() throw() {};
    const char* what() const throw() { return msg.c_str(); }
 private:
    std::string msg;
};

// used as macro in order to insert __FILE__ and __LINE__
#define THROW_EXCEPTION(msg) throw FileLineException(msg, __FILE__, __LINE__)

// ensure that i is within a range to avoid segmentation fault
#define ENSURE_BOUNDS(i, min, max)	                                	\
    (i >= min && i <= max ) ? i : THROW_EXCEPTION("ENSURE_BOUNDS failed")

#endif