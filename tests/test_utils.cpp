#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include "utils.h"

class TestUtils : public QObject
{
    Q_OBJECT

private slots:
    void sortKernelVersions_descending();
    void sortKernelVersions_ascending();
    void sortKernelVersions_mixedFormats();
    void sortKernelVersions_unmatchedFallback();
    void sortKernelVersions_singleElement();
    void sortKernelVersions_empty();

    void extractDisk_sata();
    void extractDisk_nvme();
    void extractDisk_mmc();
    void extractDisk_virtio();
    void extractDisk_wholeDisk();

    void resolveKernelFiles_debian();
    void resolveKernelFiles_archPairs();
    void resolveKernelFiles_archMissingInitramfs();

    void sanitizePathComponent_data();
    void sanitizePathComponent();
};

void TestUtils::sortKernelVersions_descending()
{
    QStringList input = {"vmlinuz-5.10.0", "vmlinuz-6.1.0", "vmlinuz-5.15.0"};
    QStringList result = utils::sortKernelVersions(input, true);
    QCOMPARE(result, QStringList({"vmlinuz-6.1.0", "vmlinuz-5.15.0", "vmlinuz-5.10.0"}));
}

void TestUtils::sortKernelVersions_ascending()
{
    QStringList input = {"vmlinuz-5.10.0", "vmlinuz-6.1.0", "vmlinuz-5.15.0"};
    QStringList result = utils::sortKernelVersions(input, false);
    QCOMPARE(result, QStringList({"vmlinuz-5.10.0", "vmlinuz-5.15.0", "vmlinuz-6.1.0"}));
}

void TestUtils::sortKernelVersions_mixedFormats()
{
    QStringList input = {"vmlinuz-6.6.87.2-microsoft-standard-WSL2", "vmlinuz-6.1.0-2-amd64", "vmlinuz-5.10.0"};
    QStringList result = utils::sortKernelVersions(input, true);
    // 6.6.87 > 6.1.0 > 5.10.0
    QCOMPARE(result.first(), QString("vmlinuz-6.6.87.2-microsoft-standard-WSL2"));
    QCOMPARE(result.last(), QString("vmlinuz-5.10.0"));
}

void TestUtils::sortKernelVersions_unmatchedFallback()
{
    QStringList input = {"zzz-noversion", "aaa-noversion"};
    QStringList result = utils::sortKernelVersions(input, true);
    QCOMPARE(result, QStringList({"zzz-noversion", "aaa-noversion"}));

    result = utils::sortKernelVersions(input, false);
    QCOMPARE(result, QStringList({"aaa-noversion", "zzz-noversion"}));
}

void TestUtils::sortKernelVersions_singleElement()
{
    QStringList input = {"vmlinuz-6.1.0"};
    QCOMPARE(utils::sortKernelVersions(input), input);
}

void TestUtils::sortKernelVersions_empty()
{
    QStringList input;
    QCOMPARE(utils::sortKernelVersions(input), QStringList());
}

void TestUtils::extractDisk_sata()
{
    QCOMPARE(utils::extractDiskFromPartition("sda1"), QString("sda"));
    QCOMPARE(utils::extractDiskFromPartition("sdb3"), QString("sdb"));
}

void TestUtils::extractDisk_nvme()
{
    QCOMPARE(utils::extractDiskFromPartition("nvme0n1p2"), QString("nvme0n1"));
    QCOMPARE(utils::extractDiskFromPartition("nvme1n1p1"), QString("nvme1n1"));
}

void TestUtils::extractDisk_mmc()
{
    QCOMPARE(utils::extractDiskFromPartition("mmcblk0p1"), QString("mmcblk0"));
}

void TestUtils::extractDisk_virtio()
{
    QCOMPARE(utils::extractDiskFromPartition("vda3"), QString("vda"));
    QCOMPARE(utils::extractDiskFromPartition("xvda1"), QString("xvda"));
}

void TestUtils::extractDisk_wholeDisk()
{
    QCOMPARE(utils::extractDiskFromPartition("sda"), QString("sda"));
    QCOMPARE(utils::extractDiskFromPartition("nvme0n1"), QString("nvme0n1"));
    QCOMPARE(utils::extractDiskFromPartition("mmcblk0"), QString("mmcblk0"));
}

void TestUtils::sanitizePathComponent_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");

    QTest::newRow("plain id") << "debian" << "debian";
    QTest::newRow("mixed case") << "MX" << "MX";
    QTest::newRow("allowed punctuation") << "open.suse-tw_1" << "open.suse-tw_1";
    QTest::newRow("space") << "MX Linux" << "MX_Linux";
    QTest::newRow("slash") << "Debian GNU/Linux" << "Debian_GNU_Linux";
    QTest::newRow("parent traversal") << "../../etc" << ".._.._etc";
    QTest::newRow("empty") << "" << "Linux";
    QTest::newRow("whitespace only") << "   " << "Linux";
    QTest::newRow("dot") << "." << "Linux";
    QTest::newRow("dot dot") << ".." << "Linux";
    QTest::newRow("dots only") << "...." << "Linux";
}

void TestUtils::sanitizePathComponent()
{
    QFETCH(QString, input);
    QFETCH(QString, expected);
    QCOMPARE(utils::sanitizePathComponent(input, "Linux"), expected);
}

static void touchFiles(const QString &dir, const QStringList &names)
{
    for (const QString &name : names) {
        QFile file(dir + '/' + name);
        QVERIFY(file.open(QIODevice::WriteOnly));
    }
}

void TestUtils::resolveKernelFiles_debian()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    touchFiles(dir.path(), {"vmlinuz-6.1.0-10-amd64", "initrd.img-6.1.0-10-amd64"});
    const auto files = utils::resolveKernelFiles(dir.path(), "6.1.0-10-amd64", false);
    QCOMPARE(files.vmlinuz, dir.path() + "/vmlinuz-6.1.0-10-amd64");
    QCOMPARE(files.initrd, dir.path() + "/initrd.img-6.1.0-10-amd64");
}

void TestUtils::resolveKernelFiles_archPairs()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    touchFiles(dir.path(), {"vmlinuz-linux", "initramfs-linux.img", "vmlinuz-linux-lts", "initramfs-linux-lts.img"});
    QCOMPARE(utils::resolveKernelFiles(dir.path(), "linux", false).initrd, dir.path() + "/initramfs-linux.img");
    QCOMPARE(utils::resolveKernelFiles(dir.path(), "linux-lts", false).initrd, dir.path() + "/initramfs-linux-lts.img");
}

void TestUtils::resolveKernelFiles_archMissingInitramfs()
{
    // The LTS kernel must not be paired with the standard kernel's initramfs
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    touchFiles(dir.path(), {"vmlinuz-linux", "initramfs-linux.img", "vmlinuz-linux-lts"});
    const auto files = utils::resolveKernelFiles(dir.path(), "linux-lts", false);
    QCOMPARE(files.vmlinuz, dir.path() + "/vmlinuz-linux-lts");
    QCOMPARE(files.initrd, dir.path() + "/initramfs-linux-lts.img");
    QVERIFY(!QFile::exists(files.initrd));
}

QTEST_MAIN(TestUtils)
#include "test_utils.moc"
