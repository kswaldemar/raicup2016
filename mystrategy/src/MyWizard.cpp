//
// Created by valdemar on 06.12.16.
//

#include "MyWizard.h"

MyWizard::MyWizard(const model::Wizard &real)
    : MyLivingUnit(MyLivingUnit::DYNAMIC,
                   real.getX(),
                   real.getY(),
                   real.getRadius(),
                   real.getLife(),
                   real.getMaxLife()),
      m_wiz(real)
{

}

double MyWizard::getMovementIncreaseFactor() const {
    return m_speed_factor;
}
