//
// Created by valdemar on 04.12.16.
//

#pragma once

#include "Vec2D.h"
#include "model/LivingUnit.h"

class MyLivingUnit {
public:

    enum Type {
        STATIC, ///< Cannot move by definition (tree or building)
        DYNAMIC, ///< Anything else
        TYPES_COUNT,
    };

    MyLivingUnit(MyLivingUnit::Type type, const model::LivingUnit &unit);

    MyLivingUnit(MyLivingUnit::Type type, double x, double y, double radius, int life, int max_life);

    Type getType() const;

    double getX() const;

    double getY() const;

    geom::Point2D getPoint() const;

    double getRadius() const;

    long long int getId() const;

    int getLife() const;

    int getMaxLife() const;

protected:
    Type m_type;
    double m_x;
    double m_y;
    double m_r;
    long long int m_id;
    int m_life;
    int m_max_life;
};


