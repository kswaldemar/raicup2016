//
// Created by valdemar on 08.11.16.
//

#pragma once

#include "InfoPack.h"
#include "model/Move.h"
/**
 * Main behaviour class, which determine wizard behaviour on the battlefield
 */
class Behaviour {
public:
    virtual void behave(model::Move &move) = 0;

    /**
     * Evaluate behaviour scoring, for choosing one which best fit to circumstances
     */
    virtual int get_score() = 0;

    /**
     * Will be called once after object creation
     * @param pack
     */
    virtual void init(const InfoPack &pack);

    /**
     * Will be called before behave function
     * @param info_pack
     */
    void update_info_pack(const InfoPack &info_pack);

protected:
    InfoPack m_i;
};