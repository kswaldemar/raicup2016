//
// Created by valdemar on 06.12.16.
//

#pragma once

#include "MyLivingUnit.h"
#include "model/Wizard.h"

class MyWizard : public MyLivingUnit {
public:
    friend class ExWorld;

    MyWizard(const model::Wizard &real);

    double getMovementIncreaseFactor() const;


protected:
    const model::Wizard &m_wiz;

    double m_speed_factor;
    double m_turn_factor;
    double m_magic_absorb_factor;
    double m_damage_absorb_factor;
};
