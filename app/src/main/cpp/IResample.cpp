//
// Created by renzhenming on 2018/10/26.
//

#include "IResample.h"
#include "XLog.h"

void IResample::Update(XData data) {
    XData d = this->Resample(data);
    XLOGI("IResample::Update %d", data.size);
    if (d.size > 0) {

        this->Notify(d);
    }
}