//
// Created by renzhenming on 2018/10/26.
//

#include "IResample.h"
void IResample::Update(XData data) {
    XData d = this->Resample(data);
    if(d.size>0){
        this->Notify(d);
    }
}