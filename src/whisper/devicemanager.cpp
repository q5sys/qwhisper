#include "devicemanager.h"
#include <QDebug>
#include <QProcess>
#include <QRegularExpression>
#include <QFile>
#include <QTextStream>

#ifdef __CUDACC__
#include <cuda_runtime.h>
#endif

DeviceManager::DeviceManager()
    : m_cudaAvailable(false)
    , m_cudaDeviceCount(0)
    , m_initialized(false)
{
    detectDevices();
}

DeviceManager& DeviceManager::instance()
{
    static DeviceManager instance;
    return instance;
}

QList<DeviceManager::DeviceInfo> DeviceManager::getAvailableDevices()
{
    if (!m_initialized) {
        detectDevices();
    }
    return m_devices;
}

bool DeviceManager::isCudaAvailable() const
{
    return m_cudaAvailable;
}

int DeviceManager::getCudaDeviceCount() const
{
    return m_cudaDeviceCount;
}

DeviceManager::DeviceInfo DeviceManager::getDeviceInfo(DeviceType type, int deviceId) const
{
    for (const auto& device : m_devices) {
        if (device.type == type && device.deviceId == deviceId) {
            return device;
        }
    }
    
    // Return default CPU if not found
    DeviceInfo cpu;
    cpu.type = CPU;
    cpu.deviceId = -1;
    cpu.name = "CPU";
    cpu.description = "System CPU";
    cpu.memorySize = 0;
    cpu.memoryFree = 0;
    cpu.isAvailable = true;
    return cpu;
}

DeviceManager::DeviceInfo DeviceManager::getDefaultDevice() const
{
    // Prefer first available CUDA device if any
    for (const auto& device : m_devices) {
        if (device.type == CUDA && device.isAvailable) {
            return device;
        }
    }
    
    // Otherwise return CPU
    return getDeviceInfo(CPU, -1);
}

QString DeviceManager::formatDeviceName(const DeviceInfo& device)
{
    if (device.type == CPU) {
        return "CPU";
    } else {
        QString name = device.name;
        if (device.memorySize > 0) {
            double memGB = device.memorySize / (1024.0 * 1024.0 * 1024.0);
            name += QString(" (%1 GB)").arg(memGB, 0, 'f', 1);
        }
        return name;
    }
}

QString DeviceManager::formatDeviceNameWithMemory(const DeviceInfo& device)
{
    QString name = device.name;
    if (device.memorySize > 0) {
        double totalGB = device.memorySize / (1024.0 * 1024.0 * 1024.0);
        double freeGB = device.memoryFree / (1024.0 * 1024.0 * 1024.0);
        name += QString(" (%1/%2 GB free)").arg(freeGB, 0, 'f', 1).arg(totalGB, 0, 'f', 1);
    } else {
        qDebug() << "formatDeviceNameWithMemory: device" << device.name << "has memorySize" << device.memorySize;
    }
    return name;
}

void DeviceManager::detectDevices()
{
    m_devices.clear();
    
    // Always add CPU as an option with system RAM info
    DeviceInfo cpu;
    cpu.type = CPU;
    cpu.deviceId = -1;
    cpu.name = "CPU";
    cpu.description = "System CPU (Default)";
    
    // Get system RAM information
    getSystemMemoryInfo(cpu.memorySize, cpu.memoryFree);
    
    // Output system RAM info in same format as GPU
    if (cpu.memorySize > 0) {
        qDebug() << "Found System RAM:" 
                 << "Memory:" << (cpu.memoryFree / (1024*1024)) << "/" 
                 << (cpu.memorySize / (1024*1024)) << "MB free";
    }
    
    cpu.isAvailable = true;
    m_devices.append(cpu);
    
    // Detect CUDA devices
    detectCudaDevices();
    
    m_initialized = true;
}

void DeviceManager::getSystemMemoryInfo(size_t& totalMemory, size_t& freeMemory)
{
    totalMemory = 0;
    freeMemory = 0;
    
    // On Linux, read from /proc/meminfo
    QFile meminfo("/proc/meminfo");
    if (meminfo.open(QIODevice::ReadOnly)) {
        // Read all content at once
        QByteArray content = meminfo.readAll();
        
        // Convert to string and split into lines
        QString allContent = QString::fromUtf8(content);
        QStringList lines = allContent.split('\n');
        
        size_t memTotal = 0;
        size_t memFree = 0;
        
        for (const QString& line : lines) {
            if (line.startsWith("MemTotal:")) {
                // Remove "MemTotal:" prefix and trim
                QString valueStr = line.mid(9).trimmed();
                
                // Split by space to separate number from "kB"
                QStringList parts = valueStr.split(' ');
                
                if (parts.size() > 0) {
                    bool ok;
                    memTotal = parts[0].toULongLong(&ok);
                    if (ok) {
                        memTotal *= 1024; // Convert from KB to bytes
                    }
                }
            } else if (line.startsWith("MemFree:")) {
                // Remove "MemFree:" prefix and trim
                QString valueStr = line.mid(8).trimmed();
                
                // Split by space to separate number from "kB"
                QStringList parts = valueStr.split(' ');
                
                if (parts.size() > 0) {
                    bool ok;
                    memFree = parts[0].toULongLong(&ok);
                    if (ok) {
                        memFree *= 1024; // Convert from KB to bytes
                    }
                }
            }
        }
        
        totalMemory = memTotal;
        freeMemory = memFree;
        
        meminfo.close();
    } else {
        qDebug() << "Could not read system memory information from /proc/meminfo";
    }
}

void DeviceManager::detectCudaDevices()
{
    m_cudaAvailable = false;
    m_cudaDeviceCount = 0;
    
#ifdef __CUDACC__
    // If compiled with CUDA support, use CUDA runtime API
    int deviceCount = 0;
    cudaError_t error = cudaGetDeviceCount(&deviceCount);
    
    if (error == cudaSuccess && deviceCount > 0) {
        m_cudaAvailable = true;
        m_cudaDeviceCount = deviceCount;
        
        for (int i = 0; i < deviceCount; ++i) {
            cudaDeviceProp prop;
            if (cudaGetDeviceProperties(&prop, i) == cudaSuccess) {
                DeviceInfo device;
                device.type = CUDA;
                device.deviceId = i;
                device.name = QString::fromLatin1(prop.name);
                device.description = QString("CUDA Device %1: %2").arg(i).arg(prop.name);
                device.memorySize = prop.totalGlobalMem;
                
                // Get free memory
                size_t freeMem = 0;
                size_t totalMem = 0;
                cudaSetDevice(i);
                cudaMemGetInfo(&freeMem, &totalMem);
                device.memoryFree = freeMem;
                
                device.isAvailable = true;
                m_devices.append(device);
                
                qDebug() << "Found CUDA device:" << device.name 
                         << "Memory:" << (device.memoryFree / (1024*1024)) << "/" 
                         << (device.memorySize / (1024*1024)) << "MB free";
            }
        }
    }
#else
    // If not compiled with CUDA, try to detect using nvidia-smi
    QProcess process;
    process.start("nvidia-smi", QStringList() << "--query-gpu=index,name,memory.total,memory.free" << "--format=csv,noheader,nounits");
    
    if (process.waitForFinished(3000)) {
        QString output = process.readAllStandardOutput();
        if (!output.isEmpty() && process.exitCode() == 0) {
            QStringList lines = output.split('\n', Qt::SkipEmptyParts);
            
            for (const QString& line : lines) {
                QStringList parts = line.split(',');
                if (parts.size() >= 4) {
                    bool ok;
                    int index = parts[0].trimmed().toInt(&ok);
                    if (ok) {
                        DeviceInfo device;
                        device.type = CUDA;
                        device.deviceId = index;
                        device.name = parts[1].trimmed();
                        
                        // Parse total memory (in MB from nvidia-smi)
                        int memoryMB = parts[2].trimmed().toInt(&ok);
                        device.memorySize = ok ? static_cast<size_t>(memoryMB) * 1024 * 1024 : 0;
                        
                        // Parse free memory (in MB from nvidia-smi)
                        int freeMemoryMB = parts[3].trimmed().toInt(&ok);
                        device.memoryFree = ok ? static_cast<size_t>(freeMemoryMB) * 1024 * 1024 : 0;
                        
                        device.description = QString("GPU %1: %2").arg(index).arg(device.name);
                        device.isAvailable = true;
                        m_devices.append(device);
                        
                        m_cudaAvailable = true;
                        m_cudaDeviceCount++;
                        
                        qDebug() << "Found NVIDIA GPU:" << device.name 
                                 << "Memory:" << freeMemoryMB << "/" << memoryMB << "MB free";
                    }
                }
            }
        }
    }
    
    // Also check if CUDA runtime is available by checking for libcudart
    if (!m_cudaAvailable) {
        QProcess ldconfig;
        ldconfig.start("ldconfig", QStringList() << "-p");
        if (ldconfig.waitForFinished(1000)) {
            QString output = ldconfig.readAllStandardOutput();
            if (output.contains("libcudart.so")) {
                qDebug() << "CUDA runtime library found but no GPUs detected";
            }
        }
    }
#endif
    
    if (!m_cudaAvailable) {
        qDebug() << "No CUDA devices found or CUDA not available";
    }
}
