#pragma once

#include <string>
#include <functional>

/**
 * Background service interface for running MySubstitute as a Windows service
 */
class BackgroundService {
public:
    BackgroundService();
    virtual ~BackgroundService();
    
    /**
     * Start the background service
     * @return true if successful
     */
    virtual bool Start();
    
    /**
     * Stop the background service
     */
    virtual void Stop();
    
    /**
     * Check if service is running
     */
    virtual bool IsRunning() const;
    
    /**
     * Set callback for status updates
     */
    void SetStatusCallback(std::function<void(const std::string&)> callback);
    
    /**
     * Get current service status
     */
    virtual std::string GetStatus() const;
    
protected:
    virtual void ServiceMain();
    virtual void OnStart();
    virtual void OnStop();
    
    bool m_running;
    std::function<void(const std::string&)> m_statusCallback;
    
private:
    void UpdateStatus(const std::string& status);
};