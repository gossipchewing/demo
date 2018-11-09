- [Introduction](#Introduction)
    - [System Requirements](#system-requirements)
- [Basic Usage](#basic-usage)
    - [Composer.json](#composer.json)
        - [The require Key](#the-require-key)
        - [How does Composer download the right files](#how-does-composer-download-the-right-files)
- [Autoloader Optimization](#autoloader-optimization)
    + [Optimization Level 1: Class map generation](#class-map-generation)
    + [Optimization Level 2/A: Authoritative class maps](#authoritative-class-maps)
    + [Optimization Level 2/B: APCu cache](#apcu-cache)

Introduction[#](#introduction)
==============================
Composer is a tool for dependency management in PHP. It allows you to declare the libraries your project depends on and it will manage (install/update) them for you.

System Requirements[#](#system-requirements)
--------------------------------------------
Composer requires PHP 5.3.2+ to run.

`Composer.json`: Project Setup[#](#composer-json-project-setup)
---------------------------------------------------------------
要在项目中使用 Composer 只需要一个 `Composer.json` 文件。该文件描述了你的项目依赖，也可能会包含其他的元数据。

### The `require` Key[#](#the-require-key)
The first (and often only) thing you specify in `composer.json` is the `require` key. You are simply telling Composer which packages your project depends on.

    {
        "require": {
            "monolog/monolog": "1.0.*"
        }
    }

Composer uses this information to search for the right set of files in package "repositories" that you register using the __`repositories`__ key, or in __Packagist__, the __default__ package repository. In the above example, since no other repository has been registered in the `composer.json` file, it is assumed that the `monolog/monolog` package is registered on Packagist.

### [How does Composer download the right files](#)(#how-does-composer-download-the-right-files)
> **How does Composer download the right files?** When you specify a dependency in `composer.json`, Composer first takes the name of the package that you have requested and searches for it in any repositories that you have registered using the [`repositories`](04-schema.md#repositories) key. If you have not registered any extra repositories, or it does not find a package with that name in the repositories you have specified, it falls back to Packagist (more [below](#packagist)).
> 
> When Composer finds the right package, either in Packagist or in a repo you have specified, it then uses the versioning features of the package's VCS (i.e., branches and tags) to attempt to find the best match for the version constraint you have specified. Be sure to read about versions and package resolution in the [versions article](articles/versions.md).
> 
> **Note:** If you are trying to require a package but Composer throws an error regarding package stability, the version you have specified may not meet your default minimum stability requirements. By default only stable releases are taken into consideration when searching for valid package versions in your VCS.
> 
> You might run into this if you are trying to require dev, alpha, beta, or RC versions of a package. Read more about stability flags and the `minimum-stability` key on the [schema page](04-schema.md).

### Updating Dependencies to their Latest Versions[#](#updating-dependencies-to-their-latest-versions)
If you only want to install or update one dependency, you can whitelist them:

    composer update monolog/monolog [...]

Platform packages[#](#platform-packages)
----------------------------------------
Composer has platform packages, which are virtual packages for things that are installed on the system __but are not actually installable by Composer__. This includes PHP itself, PHP extensions and some system libraries. 
—— 平台包是不能通过 Composer 安装的。 它们可以用来作为环境的一些限制（使用 composer 时，如果不满足这些指定的限制，composer 会给出提示）。

*   `php` represents the PHP version of the user, allowing you to apply constraints, e.g. `^7.1`. To require a 64bit version of php, you can require the `php-64bit` package.
    
*   `hhvm` represents the version of the HHVM runtime and allows you to apply a constraint, e.g., `^2.3`.
    
*   `ext-<name>` allows you to require PHP extensions (includes core extensions). Versioning can be quite inconsistent here, so it's often a good idea to set the constraint to `*`. An example of an extension package name is `ext-gd`.
    
*   `lib-<name>` allows constraints to be made on versions of libraries used by PHP. The following are available: `curl`, `iconv`, `icu`, `libxml`, `openssl`, `pcre`, `uuid`, `xsl`.

You can use `show --platform` to get a list of your locally available platform packages.

Autoloader Optimization[#](autoloader-optimization)
---------------------------------------------------
