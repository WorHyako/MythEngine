#pragma once

class IResource
{
protected:
	IResource() = default;
	virtual ~IResource() = default;

public:
    // Non-copyable and non-movable
    IResource(const IResource&) = delete;
    IResource(const IResource&&) = delete;
    IResource& operator=(const IResource&) = delete;
    IResource& operator=(const IResource&&) = delete;
};