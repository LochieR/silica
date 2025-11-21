#pragma once

#include <memory>

namespace silica {

    class Resource : public std::enable_shared_from_this<Resource>
    {
    public:
        virtual ~Resource() = default;
        
        bool isValid() const noexcept { return m_Valid; }
    protected:
        virtual void destroy() {}
        virtual void invalidate() noexcept {}
    protected:
        bool m_Valid = true;
        
        friend class VulkanInstance;
        friend class VulkanDevice;
    };

}
