#ifndef EXCEPTIONMODEL_H
#define EXCEPTIONMODEL_H

#include "exception.h"

class ExceptionModel
{
public:
    ExceptionModel();

    int insertException(const Exception &exception);
};

#endif // EXCEPTIONMODEL_H
