
#include "Manager.h"
#include "curl/curl.h"

namespace fs = std::filesystem;

namespace snowball {
namespace pm {

Manager::Manager(toml::parse_result p_package, bool p_silent, std::string p_cwd, std::string p_configFolder) {
  package = p_package;
  silent = p_silent;
  cwd = p_cwd;
  configFolder = p_configFolder;
}

std::string Manager::getGit() {
  std::string git = "git";
  if (getenv("SNOWBALL_GIT_PATH") != NULL) {
    git = getenv("SNOWBALL_GIT_PATH");
  }
  return git;
}

int Manager::runAsMain() {
  if (!package["dependencies"]) return EXIT_SUCCESS;
  (void)chdir(((fs::path)configFolder/"deps").c_str());

  // example: "owner/repo" = { version = "1.0.0" }
  auto packages = package["dependencies"].as_table();
  
  packages->for_each([&](auto&& key, auto&& value) {
    auto repo = (std::string)key.str();
    auto packageData = package["dependencies"][repo];

    Package packageInfo;
    packageInfo.name = repo;
    packageInfo.version = packageData["version"].value_or<std::string>("latest");
  
    auto result = install(packageInfo);
    if (result != EXIT_SUCCESS) {
      throw SNError(Error::PM_ERROR, FMT("Failed to install package %s", repo.c_str()));
    }
  });

  (void)chdir(cwd.c_str());

  return EXIT_SUCCESS;
}

bool Manager::isInstalled(Package p_package) {
  // check on folder structure
  auto packageFolder = (fs::path)configFolder / "deps" / p_package.name;
  return fs::is_directory(packageFolder);
}

int Manager::install(Package p_package) {
  if (isInstalled(p_package)) return EXIT_SUCCESS;

  std::string git = getGit();
  auto pkgInfo = getPackageInfoFromRepo(p_package.name);

  if (!silent) {
    Logger::message("Downloading", FMT(" Package %s%s%s from git repository", BOLD, p_package.name.c_str(), RESET));
  }

  auto packageFolder = (fs::path)configFolder / "deps" / p_package.name;
  std::string downloadUrl = pkgInfo["download_url"];
  if (downloadUrl.empty()) {
    throw SNError(Error::PM_ERROR, FMT("Failed to install package '%s'. No download_url found in package info", p_package.name.c_str()));
  }

  auto gitCommand = std::vector<std::string>{};
  gitCommand.push_back(git);
  gitCommand.push_back("clone");
  gitCommand.push_back(downloadUrl);
  gitCommand.push_back(packageFolder);
  gitCommand.push_back("--depth=1");
  gitCommand.push_back("--quiet");
  auto status = os::Driver::run(gitCommand);  

  if (status != EXIT_SUCCESS) {
    throw SNError(Error::PM_ERROR, FMT("Failed to install package '%s'. See the output above", p_package.name.c_str()));
  }

  return EXIT_SUCCESS;
}

namespace {
std::size_t writeCallback(
  const char* in,
  std::size_t size,
  std::size_t num,
  std::string* out)
{
  const std::size_t totalBytes(size * num);
  out->append(in, totalBytes);
  return totalBytes;
}
}

nlohmann::json Manager::getPackageInfoFromRepo(std::string p_repo) {
  auto packageInfo = nlohmann::json{};
  std::unique_ptr<std::string> httpData(new std::string());

  auto curl = curl_easy_init();
  if (curl) {
    auto url = _SNOWBALL_PACKAGE_REGISTRY + p_repo + ".json";
    long httpCode(0);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (compatible; MSIE 10.0; Windows NT 6.0; Win64; x64 Trident/6.0)");
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
    // Don't wait forever, time out after 10 seconds.
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
    // Acces https without ca verification
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, httpData.get());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    if (!silent) {
      Logger::message("Fetching", FMT("Package info for %s%s%s from registry", BOLD, p_repo.c_str(), RESET));
    }
    auto curlCode = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    
    curl_easy_cleanup(curl);
    if (curlCode != CURLE_OK) {
      throw SNError(Error::PM_ERROR, FMT("Failed to get package info from registry. CURL code %s", curl_easy_strerror(curlCode)));
    }

    if (httpCode != 200) {
      throw SNError(Error::PM_ERROR, FMT("Failed to get package info from registry. HTTP code %d", httpCode));
    }
  } else {
    throw SNError(Error::PM_ERROR, "Failed to initialize curl");
  }

  try {
    packageInfo = nlohmann::json::parse(*httpData);
  } catch (const std::exception& e) {
    throw SNError(Error::PM_ERROR, FMT("Failed to parse package info from registry. %s", e.what()));
  }
  return packageInfo;
}

} // namespace pm
} // namespace snowball
