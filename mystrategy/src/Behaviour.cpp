//
// Created by valdemar on 08.11.16.
//

#include "Behaviour.h"

void Behaviour::update_info_pack(const InfoPack &info_pack) {
    //Will copy only pointers, so it's ok
    m_i = info_pack;
}

void Behaviour::init(const InfoPack &pack) {
    //Nothing by default
}
