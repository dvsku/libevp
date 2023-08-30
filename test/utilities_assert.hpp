#ifndef UTILITIES_ASSERT_HPP
#define UTILITIES_ASSERT_HPP

// Assert that a statement is true
#define ASSERT(expression) {                                                                        \
    if(!(expression)) {                                                                             \
        printf("assertion '" #expression "' failed at line %d in file %s\n", __LINE__, __FILE__);   \
        exit(EXIT_FAILURE);                                                                         \
    }                                                                                               \
}

// Use to check that a statement throws exceptions.
#define ASSERT_THROWS(expression) {                                                                 \
    try {                                                                                           \
        expression;                                                                                 \
        printf("assertion '" #expression "' failed at line %d in file %s\n", __LINE__, __FILE__);   \
        exit(EXIT_FAILURE);                                                                         \
    }                                                                                               \
    catch (...) { }                                                                                 \
}

// Use to check that a statement doesn't throw exceptions.
#define ASSERT_NOT_THROWS(expression) {                                                             \
    try {                                                                                           \
        expression;                                                                                 \
    }                                                                                               \
    catch (...) {                                                                                   \
        printf("assertion '" #expression "' failed at line %d in file %s\n", __LINE__, __FILE__);   \
        exit(EXIT_FAILURE);                                                                         \
    }                                                                                               \
}

// Use to check that construction of an object doesn't throw exceptions.
// Create that object after if it doesn't.
#define ASSERT_NOT_THROWS_CTOR(expression) {                                                        \
    try {                                                                                           \
        expression;                                                                                 \
    }                                                                                               \
    catch (...) {                                                                                   \
        printf("assertion '" #expression "' failed at line %d in file %s\n", __LINE__, __FILE__);   \
        exit(EXIT_FAILURE);                                                                         \
    }                                                                                               \
}                                                                                                   \
    expression;                                                                                     

#endif
