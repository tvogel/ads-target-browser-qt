#pragma once

#include <QString>
#include <optional>

bool adsAddRouteToPLC(
    const QString & sending_net_id,
    const QString & adding_host_name,
    const QString & ip_address,
    const QString & username,
    const QString & password,
    std::optional<QString> route_name = std::nullopt,
    std::optional<QString> added_net_id = std::nullopt);
