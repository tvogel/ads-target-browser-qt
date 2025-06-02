#include "AdsSymbolUploadInfo2.h"
#include "AdsDevice.h"

#include <QDebug>

AdsSymbolUploadInfo2 AdsSymbolUploadInfo2::fromDevice(AdsDevice & device)
{
    AdsSymbolUploadInfo2 info;
    auto error = device.ReadReqEx2(ADSIGRP_SYM_UPLOADINFO2, 0, sizeof(info), &info, nullptr);
    if (error) {
        throw AdsException(error);
    }
    return info;
}

QByteArray AdsSymbolUploadInfo2::uploadSymbols(AdsDevice & device) const
{
    auto symbols = QByteArray(nSymSize, Qt::Uninitialized);
    uint32_t bytesRead = 0;
    auto error = device.ReadReqEx2(ADSIGRP_SYM_UPLOAD, 0, nSymSize, symbols.data(), &bytesRead);
    if (error) {
        throw AdsException(error);
    }
    if (bytesRead != nSymSize) {
      qWarning() << "uploadSymbols: Expected to read" << nSymSize << "bytes, but only read" << bytesRead << "bytes.";
    }
    return symbols;
}

QByteArray AdsSymbolUploadInfo2::uploadDatatypes(AdsDevice & device) const
{
    auto datatypes = QByteArray(nDatatypeSize, Qt::Uninitialized);
    uint32_t bytesRead = 0;
    auto error = device.ReadReqEx2(ADSIGRP_SYM_DT_UPLOAD, 0, nDatatypeSize, datatypes.data(), &bytesRead);
    if (error) {
        throw AdsException(error);
    }
    if (bytesRead != nDatatypeSize) {
      qWarning() << "uploadDatatypes: Expected to read" << nDatatypeSize << "bytes, but only read" << bytesRead << "bytes.";
    }
    return datatypes;
}
