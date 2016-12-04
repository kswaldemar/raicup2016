//
// Created by valdemar on 04.12.16.
//

#pragma once


#include "model/LivingUnit.h"

class MyLivingUnit {
public:

    enum Type {
        STATIC, ///< Cannot move by definition (tree or building)
        DYNAMIC, ///< Anything else
        TYPES_COUNT,
    };

    MyLivingUnit(MyLivingUnit::Type type, const model::LivingUnit &unit);

    Type getType() const;
    double getX() const;
    double getY() const;
    double getRadius() const;
    long long int getId() const;
    int getLife() const;
    int getMaxLife() const;

private:
    Type m_type;
    double m_x;
    double m_y;
    double m_r;
    long long int m_id;
    int m_life;
    int m_max_life;
};


