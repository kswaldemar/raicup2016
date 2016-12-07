//
// Created by valdemar on 07.12.16.
//

#include "BulletHandler.h"
#include "VisualDebug.h"
#include "Logger.h"
#include <algorithm>
#include <cassert>

//BulletDesc::BulletDesc() {
//
//}

BulletDesc::BulletDesc(const model::Projectile &bullet, double cast_dist) {
    id = bullet.getId();
    mv_dir = {bullet.getSpeedX(), bullet.getSpeedY()};
    spd_mod = mv_dir.len();
    cur_pos = {bullet.getX(), bullet.getY()};
    //Assume projectile was created at previous tick
    st = cur_pos - mv_dir;
    mv_dir = geom::normalize(mv_dir);
    fin = st + mv_dir * cast_dist;

    owner_id = bullet.getOwnerUnitId();
    radius = bullet.getRadius();
}

void BulletDesc::update(const model::Projectile &bullet) {
    cur_pos = {bullet.getX(), bullet.getY()};
}

void BulletHandler::update_projectiles(const model::World &world, const model::Game &game) {

    //Record all visible projectiles
    std::set<long long int> visible;
    for (const auto &i : world.getProjectiles()) {
        visible.insert(i.getId());
    }

    //Remove all projectiles out of visible zone
    for (auto it = m_bullets.begin(); it != m_bullets.end();) {
        if (visible.find(it->id) == visible.end()) {
            it = m_bullets.erase(it);
        } else {
            ++it;
        }
    }

    const int my_faction = world.getMyPlayer().getFaction();
    std::set<long long int> friendly_projectiles;
    for (const auto &i : world.getMinions()) {
        if (i.getType() == model::MINION_FETISH_BLOWDART && i.getFaction() == my_faction) {
            friendly_projectiles.insert(i.getId());
        }
    }
    for (const auto &i : world.getWizards()) {
        if (i.getFaction() == my_faction) {
            friendly_projectiles.insert(i.getId());
        }
    }



    for (const auto &i : world.getProjectiles()) {
        if (friendly_projectiles.find(i.getOwnerUnitId()) != friendly_projectiles.end()) {
            //Hope it will not hurt me
            continue;
        }
        auto it = std::find_if(m_bullets.begin(), m_bullets.end(),
                               [&i](const BulletDesc &desc) {
                                   return desc.id == i.getId();
                               });
        if (it != m_bullets.end()) {
            it->update(i);
        } else {
            //Find owner
            double cast_range = -1;

            if (i.getType() == model::PROJECTILE_DART) {
                cast_range = game.getFetishBlowdartAttackRange();
            } else {
                for (const auto &owner : world.getWizards()) {
                    if (i.getOwnerUnitId() == owner.getId()) {
                        cast_range = owner.getCastRange();
                        break;
                    }
                }
            }
            if (cast_range < 0) {
                //See projectiles casted from outside FOV
                cast_range = game.getWizardCastRange();
            }
            m_bullets.emplace_back(i, cast_range);
        }
    }
}

void BulletHandler::visualise_projectiles() const {
    for (const auto &i : m_bullets) {
        VISUAL(line(i.st.x, i.st.y, i.fin.x, i.fin.y, 0xC70A82));
        VISUAL(fillCircle(i.cur_pos.x, i.cur_pos.y, i.radius, 0xF145B2));
    }
}

HitDescription
BulletHandler::calc_hit_description(const BulletDesc &bullet, const geom::Point2D &obj, double radius) {
    using namespace geom;
    HitDescription ret;
    Vec2D a = obj - bullet.cur_pos;
    const double a_len = a.len();
    const double cosa = a * bullet.mv_dir / a_len;
    const double angle = acos(cosa);

    ret.hit_pt = {-1, -1};

    bool hit = true;
    if (angle > pi / 2) {
        //Out of bullet range
        ret.ticks = -1;
        ret.dist = -1;
        hit = false;
    } else {
        const double sina = sqrt(1.0 - cosa * cosa);
        const double h = a_len * sina;
        if (h > bullet.radius + radius) {
            //Will not hit
            hit = false;
            ret.dist = h;
        }
        const double d = a_len * cosa;
        const double trace_len = (bullet.fin - bullet.cur_pos).len();
        if (d <= trace_len) {
            //Will touch object in middle of projectile trace
            ret.dist = h;
            ret.ticks = static_cast<int>(ceil(d / bullet.spd_mod));
            ret.hit_pt = bullet.cur_pos + bullet.mv_dir * d;
        } else {
            //Near end of trace
            ret.dist = (obj - bullet.fin).len();
            ret.hit_pt = bullet.cur_pos + bullet.mv_dir * trace_len;
            if (ret.dist <= bullet.radius + radius) {
                ret.ticks = static_cast<int>(ceil(trace_len / bullet.spd_mod));
            } else {
                hit = false;
            }
        }
    }

    if (!hit) {
        ret.ticks = -1;
    }

    return ret;
}

void BulletHandler::check_projectiles_hit(const geom::Point2D &obj, double radius) const {
    for (const auto &i : m_bullets) {
        auto hds = calc_hit_description(i, obj, radius);
        if (hds.ticks > 0) {
            LOG("Projectile HIT: distance = %3.3lf; time = %d\n", hds.dist, hds.ticks);
            VISUAL(fillCircle(hds.hit_pt.x, hds.hit_pt.y, i.radius, 0x80D600));
            VISUAL(fillCircle(i.cur_pos.x, i.cur_pos.y, i.radius, 0xFF0000));
        } else {
            LOG("Projectile MISS: distance = %3.3lf; time = %d\n", hds.dist, hds.ticks);
        }
    }
}

const std::list<BulletDesc> &BulletHandler::get_bullets() const {
    return m_bullets;
}
