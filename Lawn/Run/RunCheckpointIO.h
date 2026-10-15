#pragma once
#include <string>
#include <vector>

// @pvz-online: a durable journal joins the otherwise separate wallet and run files.
namespace RunCheckpointIO {
bool AtomicWrite(const std::string& path, const std::vector<unsigned char>& bytes);
bool Recover(const std::string& runPath, const std::string& userPath);
bool Commit(const std::string& runPath, const std::string& userPath,
            const std::vector<unsigned char>& run, const std::vector<unsigned char>& user);
}
