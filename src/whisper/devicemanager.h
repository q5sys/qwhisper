#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QString>
#include <QList>
#include <QPair>
#include <cstddef>

class DeviceManager
{
public:
    enum DeviceType {
        CPU = 0,
        CUDA = 1
    };
    
    struct DeviceInfo {
        DeviceType type;
        int deviceId;           // -1 for CPU, 0+ for GPU index
        QString name;
        QString description;
        size_t memorySize;      // Total memory in bytes (0 for CPU)
        size_t memoryFree;      // Free memory in bytes (0 for CPU)
        bool isAvailable;
    };
    
    static DeviceManager& instance();
    
    // Get list of all available devices
    QList<DeviceInfo> getAvailableDevices();
    
    // Check if CUDA is available
    bool isCudaAvailable() const;
    
    // Get number of CUDA devices
    int getCudaDeviceCount() const;
    
    // Get device info by ID
    DeviceInfo getDeviceInfo(DeviceType type, int deviceId) const;
    
    // Get default device (first CUDA if available, otherwise CPU)
    DeviceInfo getDefaultDevice() const;
    
    // Format device name for display
    static QString formatDeviceName(const DeviceInfo& device);
    
    // Format device name with memory info for display
    static QString formatDeviceNameWithMemory(const DeviceInfo& device);
    
private:
    DeviceManager();
    ~DeviceManager() = default;
    DeviceManager(const DeviceManager&) = delete;
    DeviceManager& operator=(const DeviceManager&) = delete;
    
    void detectDevices();
    void detectCudaDevices();
    void getSystemMemoryInfo(size_t& totalMemory, size_t& freeMemory);
    
    QList<DeviceInfo> m_devices;
    bool m_cudaAvailable;
    int m_cudaDeviceCount;
    bool m_initialized;
};

#endif // DEVICEMANAGER_H
