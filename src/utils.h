#pragma once

#include <QStringList>

namespace utils
{

struct KernelFiles {
    QString vmlinuz;
    QString initrd;
    QString amdUcode;
    QString intelUcode;
};

KernelFiles resolveKernelFiles(const QString &sourceDir, const QString &kernelVersion, bool isFrugal);
QStringList sortKernelVersions(const QStringList &kernelFiles, bool reverse = true);
QString extractDiskFromPartition(const QString &partition);
// Reduce a name to a single safe directory component ([A-Za-z0-9._-]), using fallback if nothing usable remains
QString sanitizePathComponent(const QString &name, const QString &fallback);

} // namespace utils
