//
// Created by valdemar on 26.11.16.
//

#pragma once

#include "Field.h"

namespace fields {

class CustomField : public IField {
public:
    using FuncType = double (*)(double x);
    CustomField(const geom::Point2D &center, double r1, double r2, FuncType fn);

    double get_value(double x, double y) const override;

private:
    double m_r1;
    double m_r2;
    FuncType m_custom_func;
};


}