//
// Created by valdemar on 07.12.16.
//

#pragma once

#include "model/World.h"
#include "model/Game.h"
#include "Vec2D.h"
#include <set>
#include <list>

struct BulletDesc {

    //BulletDesc(const geom::Point2D &start, const geom::Point2D &fin, );

    BulletDesc(const model::Projectile &bullet, double cast_dist);

    void update(const model::Projectile &bullet);

    geom::Point2D st;
    geom::Point2D cur_pos;
    geom::Point2D fin;
    geom::Vec2D mv_dir;
    double spd_mod;
    double radius;
    long long int id;
    long long int owner_id;
};


struct HitDescription {
    //Remaining time before hit, -1 if never
    int ticks;
    //Distance to closest point on trace
    double dist;
    //Point from which bullet will hurt us
    geom::Point2D hit_pt;
};

class BulletHandler {
public:
    void update_projectiles(const model::World &world, const model::Game &game);

    void visualise_projectiles() const;

    static HitDescription calc_hit_description(const BulletDesc &bullet, const geom::Point2D &obj, double radius);

    void check_projectiles_hit(const geom::Point2D &obj, double radius) const;

    const std::list<BulletDesc> &get_bullets() const;

private:
    std::list<BulletDesc> m_bullets;
};
