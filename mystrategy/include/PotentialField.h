//
// Created by valdemar on 09.11.16.
//

#pragma once

#include "InfoPack.h"
#include "Point2D.h"
#include <cstdint>
#include <vector>
#include <model/Unit.h>
#include <model/Minion.h>
#include <model/Building.h>

/**
 * Class representing potential field
 * 0 - calm field, no tension
 * >0 - attraction
 * <0 - push out, distraction
 */
class PotentialField {
public:

    static const std::initializer_list<Point2D> m_around_shift;

    struct FieldVal {

        bool operator>(const FieldVal &other) const {
            return val > other.val;
        }

        //Force in current point
        int val = 0;
        //Desired view angle
        double angle = 0;
        //If true, angle should be ignored
        bool any_angle = true;
    };


    PotentialField(uint32_t width, uint32_t height, uint32_t scale);

    /*
     * Decrease field values
     */
    void relax();

    /*
     * Add unit to potential field
     * Adjust field value in particular range, depending on unit type, faction, statuses, hp level, our hp level etc.
     * Main idea to do all calculations with that function and next just move by potentials
     */
    void add_unit(const model::Wizard &wizard);
    void add_unit(const model::Minion &creep);
    void add_unit(const model::Building &build);

    /*
     * Add waypoint to potential field
     */
    void add_waypoint(const Point2D &wp);

    /*
     * Update self, may be used in field calculations
     */
    void update_self(const model::Wizard &self);

    /*
     * Apply potential field to unit coordinate
     */
    Point2D potential_move(const Point2D &unit);

    /*
     * Get corresponding potential field cell value. Automatically scale coordinates
     */
    FieldVal * get_cell_value(const Point2D &pt);

private:
    /*
     * Cleanup field to zero state
     */
    void clean();

    void clean_visited();


    ///Real field; X Y
    std::vector<std::vector<FieldVal>> m_f;
    int m_width;
    int m_height;
    //Pointer to myself
    const model::Wizard *m_self;
    //Real world to field scaling
    uint32_t m_scale;
    //Supplementary flags vector
    std::vector<std::vector<bool>> m_visited;
};
