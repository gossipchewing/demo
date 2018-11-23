- [Introduction](#introduction)
    - [System Requirements](#system-requirements)
- [Basic Usage](#basic-usage)
    - [composer.json](#composer-json)
        + [The require Key](#the-require-key)
        + [How does Composer download the right files](#how-does-composer-download-the-right-files)
    - [Installing Dependencies](#installing-dependencies)
        + [Installing Without composer.lock](#installing-without-composer-lock)
        + [Installing With composer.lock](#installing-with-composer.lock)
- [Updating Dependencies](#updating-dependencies)
- [Platform packages](#platform-packages)
- [Autoloader Optimization](#autoloader-optimization)
    + [Optimization Level 1: Class map generation](#class-map-generation)
    + [Optimization Level 2/A: Authoritative class maps](#authoritative-class-maps)
    + [Optimization Level 2/B: APCu cache](#apcu-cache)
- [配置 Composer 使用本地仓库](#配置-composer-使用本地仓库)
    + [VCS](#vcs)
    + [Path](#path)
- [版本限制](#版本限制)
    + [VCS Tags and Branches](#vcs-tags-and-branches)
    + [Composer & tag & branch](#Composer-&-tag-&-branch)
    + [Examples](#examples)

Introduction
============
Composer 是一个 PHP 的依赖管理工具。你可以通过声明项目的依赖，然后，就可以用 Composer 进行管理了。

System Requirements
-------------------
PHP 5.3.2+.

Basic Usage
-----------

## composer.json
要在项目中使用 Composer 只需要一个 `Composer.json` 文件。该文件描述了你的项目依赖，也可能会包含其他的元数据。

### The require Key
The first (and often only) thing you specify in `composer.json` is the `require` key. You are simply telling Composer which packages your project depends on.

    {
        "require": {
            "monolog/monolog": "1.0.*"
        }
    }

Composer uses this information to search for the right set of files in package "repositories" that you register using the __`repositories`__ key, or in __Packagist__, the __default__ package repository. In the above example, since no other repository has been registered in the `composer.json` file, it is assumed that the `monolog/monolog` package is registered on Packagist.

### How does Composer download the right files
>When you specify a dependency in `composer.json`, Composer first takes the name of the package that you have requested and searches for it in any repositories that you have registered using the [`repositories`](04-schema.md#repositories) key. If you have not registered any extra repositories, or it does not find a package with that name in the repositories you have specified, it falls back to Packagist (more [below](#packagist)).
> 
> When Composer finds the right package, either in Packagist or in a repo you have specified, it then uses the versioning features of the package's VCS (i.e., branches and tags) to attempt to find the best match for the version constraint you have specified. Be sure to read about versions and package resolution in the [versions article](articles/versions.md).
> 
> **Note:** If you are trying to require a package but Composer throws an error regarding package stability, the version you have specified may not meet your default minimum stability requirements. By default only stable releases are taken into consideration when searching for valid package versions in your VCS.
> 
> You might run into this if you are trying to require dev, alpha, beta, or RC versions of a package. Read more about stability flags and the `minimum-stability` key on the [schema page](04-schema.md).

## Installing Dependencies
    composer install
执行该命令后，根据当前目录是否有 composer.lock 文件，Composer 的行为会有两种：
- Installing without composer.lock
    + 直接根据 `composer.json` 文件安装所需的依赖的指定版本。
    + 安装完成后，将已下载的 package 及其确切版本写入 `composer.lock`，将项目锁定到这些特定版本。 所以，如果使用 VCS，应该讲该文件提交。

- Installing with composer.lock
    + 在安装 `composer.json` 中指定的依赖时，Composer 会解析  `composer.lock` 文件，如果有对应的记录，就会使用里面记录的版本号。如果没有，才会使用  `composer.json` 中指定的版本。 这样可以确保整个项目都使用相同的依赖版本。

### Updating Dependencies
If you only want to install or update one dependency, you can whitelist them:

    composer update monolog/monolog [...]

Platform packages
-----------------
Composer has platform packages, which are virtual packages for things that are installed on the system __but are not actually installable by Composer__. This includes PHP itself, PHP extensions and some system libraries. 
—— 平台包是不能通过 Composer 安装的。 它们可以用来作为环境的一些限制（使用 composer 时，如果不满足这些指定的限制，composer 会给出提示）。

*   `php` represents the PHP version of the user, allowing you to apply constraints, e.g. `^7.1`. To require a 64bit version of php, you can require the `php-64bit` package.
    
*   `hhvm` represents the version of the HHVM runtime and allows you to apply a constraint, e.g., `^2.3`.
    
*   `ext-<name>` allows you to require PHP extensions (includes core extensions). Versioning can be quite inconsistent here, so it's often a good idea to set the constraint to `*`. An example of an extension package name is `ext-gd`.
    
*   `lib-<name>` allows constraints to be made on versions of libraries used by PHP. The following are available: `curl`, `iconv`, `icu`, `libxml`, `openssl`, `pcre`, `uuid`, `xsl`.

You can use `show --platform` to get a list of your locally available platform packages.

Autoloader Optimization
-----------------------
## Class map generation
默认情况下加载一个类时，必须要根据 PSR 的规则，去查找类的位置。如果我们就知道了类名和它所在位置的映射关系，就能很快定位到要找的类，class map 就是这么做的（当然，如果没有在 class map 中找到指定的包，还是会根据 PSR 规则去找）。有三种方式可以生成 class map:

* 在 `composer.json` 中配置： `"optimize-autoloader": true`
* install 或 update 时带上 `-o / --optimize-autoloader`
* 命令行执行 `composer dump-autoload` 时带上 `-o / --optimize`

对 php5.6+ 版本，这个 class map 会被缓存到 opcache 中。

## Authoritative class maps
这种情况下，只会从生成的 class map 中查找类，没找到，就不再根据 PSR 规则查找了。 同样，有三种方式指定：
* 在 `composer.json` 中配置： `"classmap-authoritative": true`
* install 或 update 时带上 `-a / --classmap-authoritative`
* 命令行执行 `composer dump-autoload` 时带上 `-a / --classmap-authoritative`

这种方式的优点是非常快。但是如果一个类时运行时生成的，该类是不能被 Composer 自动加载的。

## APCu cache
执行命令后，这种方式并不会立即生成 class map。而是找不到一个就根据规则查找它，找到后，再将它放入到 class map 中，这样的话，后续的加载就能直接在 class map 中找到了。 使用前提：要安装了 APCu。 

* 在 `composer.json` 中配置： `"apcu-autoloader": true`
* install 或 update 时带上 `--apcu-autoloader`
* 命令行执行 `composer dump-autoload` 时带上 `-apcu`

配置 Composer 使用本地仓库
------------------------
## VCS
依赖指定为本地 git 仓库。

1、在被依赖的 package 中配置好 `composer.json` 文件。假定这里有两个要引入的依赖 sms 和 email。
在 sms 根目录下：
```
{
    "name": "service/sms" // 必须指定，后面的依赖中要用。命令行 require 也是用的这个
}
```
同样的，email：
```
{
    "name": "service/email"
}
```

2、在项目中引入依赖
```
{
  "name": "test/app",
  "type": "project",
  "repositories": [
    {
      "type": "git", // 这里是 git 或 vcs 都行（测试用的 git）
      "url": "git@gitlab.private.com:service/email.git" // 被依赖包的 git 地址
    },
    {
      "type": "vcs",
      "url": "git@gitlab.private.com:service/sms.git"
    }
  ],
  "require": {
    "service/email": "dev-master",  // 根据步骤一中配置的名称引入。对应的实际上是 master 分支，dev- 是 Composer 的约定
    "service/sms": "dev-master"
  }
}
```
这样配置就 OK 了，`composer install` 就可以成功安装啦~

对于配置中的 require 部分，可以在命令行指定。例如： `composer require service/sms:dev-master` 就可以引入对 sms 的 master 分支的依赖。

## Path
依赖指定为一个本地目录(这里用 package 描述该目录)。假定有如下：
```
- apps
\_ my-app
  \_ composer.json
- packages
\_ my-package
  \_ composer.json
```

If the package is a local VCS repository, the version may be inferred by the branch or tag that is currently checked out. Otherwise, the version should be explicitly defined in the package's `composer.json` file. If the version cannot be resolved by these means, it is assumed to be `dev-master`.

my-package/composer.json（json 中不能有注释，下面的只做示意用）
如果不是 VCS repository：
```
{
    "name": "my/package", // 必须
    "type": "library",
    "description": "composer demo",
    "version": "1.0.1" // 如果不是 VCS repository 则必须显示指定版本！
}
```
使用 git init 初始化一下 my-package，那么，就可以使用一个最简单的配置：
```
{
    "name":"my/package"
}
```
my-app/composer.json
```
{
    "name": "test/app",
    "type": "project",
    "repositories": [
    {
        "type": "path",
        "url": "C:\\nginx\\html\\localhost\\Composer\\packages\\my-package"
        "url": "../../packages/*", // url 也可以指定为相对路径
        "options": {
            "symlink": true // 引入依赖包时以软链接的方式指向。 false 则会直接复制依赖包过来。
        }
    }
    ],
    "require": {
        "my/package": "@dev"   // 必须指定
    }
}
```

命令行：
```
$ composer config --list
[repositories.0.type] path
[repositories.0.url] ../../packages/*
[repositories.packagist.org.type] composer
[repositories.packagist.org.url] https://packagist.phpcomposer.com
[process-timeout] 300
[use-include-path] false
[preferred-install] auto
[notify-on-install] true
[github-protocols] [https, ssh]
[vendor-dir] vendor (C:\nginx\html\localhost\Composer\Path\apps\my-apps/vendor)
[bin-dir] {$vendor-dir}/bin (C:\nginx\html\localhost\Composer\Path\apps\my-apps/vendor/bin)
[cache-dir] C:/Users/crystal.yao/AppData/Local/Composer
[data-dir] C:/Users/crystal.yao/AppData/Roaming/Composer
[cache-files-dir] {$cache-dir}/files (C:/Users/crystal.yao/AppData/Local/Composer/files)
[cache-repo-dir] {$cache-dir}/repo (C:/Users/crystal.yao/AppData/Local/Composer/repo)
[cache-vcs-dir] {$cache-dir}/vcs (C:/Users/crystal.yao/AppData/Local/Composer/vcs)
[cache-ttl] 15552000
[cache-files-ttl] 15552000
[cache-files-maxsize] 300MiB (314572800)
[bin-compat] auto
[discard-changes] false
[autoloader-suffix]
[sort-packages] false
[optimize-autoloader] false
[classmap-authoritative] false
[apcu-autoloader] false
[prepend-autoloader] true
[github-domains] [github.com]
[bitbucket-expose-hostname] true
[disable-tls] false
[secure-http] true
[cafile]
[capath]
[github-expose-hostname] true
[gitlab-domains] [gitlab.com]
[store-auths] prompt
[archive-format] tar
[archive-dir] .
[home] C:/Users/crystal.yao/AppData/Roaming/Composer
```

版本限制
-------
### 使用通配符

### 使用 `~` 限定版本范围. （限定后，指定的限定规则的最后一位可以向上变动）

例子：

`~1.2` 相当于 `>=1.2 <2.0.0`  --- 主版本号 1 固定

`~1.2.3` 相当于 `>=1.2.3 <1.3.0` --- 主次版本号 1.2 固定

注意：

*  虽然 2.0-beta.1 严格来说是在 2.0 之前，但像 ~1.2 这样的版本约束不会安装它。因为 __~1.2 仅表示 .2 可以改变，但 1. 部分是固定的__。

*  `~` 运算符对主要版本号的行为有例外。例如 ~1 与 ~1.0 相同，因为它不允许主版本号增加以试图保持向后兼容性。

### 使用 `^` 限定版本范围（限定后，分两种情况，1.0 之前和之后）

`^1.2.3` 相当于 `>=1.2.3 <2.0.0`，因为在 2.0 之前的任何版本都不应该破坏向后兼容性。

对于 1.0 之前的版本，它也考虑到安全性，并将 `^0.3` 视为 `>=0.3.0 <0.4.0`。

>在编写库代码时，这是推荐的操作符，可实现最大的互操作性。


## VCS Tags and Branches
tag 是指向 Git 历史中特定点的 ref 标记。 tag 通常用于标记版本发布的节点（如 v1.0.1）。

*  tag 指向一个具体的 commit，它跟分支没有任何关系。也就是说，它只是用来标识单个指定的 commit。 A tag is a pointer to a commit, and commits exist independently of branches.
*  Tags and branch are completely unrelated, since tags refer to a specific commit, and branch is a moving reference to the last commit of a history. Branches go, tags stay（branch 是动态的，tag 是静态的）.
*  在使用 `git tag <tagname>` 命令时，分支和标记有了间接的关系。即，此时，tag 是指向当前分支的 `HEAD`。 也可以指定分支，比如： `git tag v1.0.1 develop` 这时，标记 `v1.0.1` 指向 `develop` 分支的 `HEAD`。 当然可以直接指定 tag 要指向的 `commit|Object ID`.
```
$ git log --pretty=oneline
aeb96d33da757584a9abad4cb79fd6ee289b1342 (HEAD -> master, tag: v1.1.1) master xxx3
...
34c273f553a49c51638fc82ac5ad152b27e7e6fb (tag: v1.0.9) master xxx
81027880b4b655a000d3b9c1992d3cacfce87ae8 master xxx

$ git log --pretty=oneline --abbrev-commit
aeb96d3 (HEAD -> master, tag: v1.1.1) master xxx3
...
34c273f (tag: v1.0.9) master xxx
8102788 master xxx
```
执行 `git tag v1.0.8 8102788` 创建标签后如下：
```
$ git log --pretty=oneline --abbrev-commit
aeb96d3 (HEAD -> master, tag: v1.1.1) master xxx3
...
34c273f (tag: v1.0.9) master xxx
8102788 (tag: v1.0.8) master xxx
```

## Composer & tag & branch
通常，Composer 处理标记（而不是分支）。在写版本约束时，它可以引用特定标记（例如，`1.1`），或者它可以引用有效范围的标记（例如，`>= 1.1 <2.0`，或 `~4.0`）。

在指定标记名称时，通常是 `v1.1.1` 这样的形式，Composer 在处理时，会自动删除实际 tag 中的 `v` 前缀，以获取有效的最终版本号。

如果希望 Composer checkout 某个分支而不是标记，则需要使用特殊的 `dev-*` 前缀（如果分支是类似于版本号形式的命名方式，例如 `v1`，则需要使用后缀 `-dev`）。

例如，如果要 checkout `my-feature` 分支： `composer require dev-my-feature`。
这将导致 Composer 将改分支所在的 repository 被克隆到我们的 vendor 目录并在该目录中 checkout 出 my-feature 分支。 这与处理 tag 的行为是不同的。
> If you're checking out a branch, it's assumed that you want to work on the branch and Composer actually clones the repo into the correct place in your vendor directory. For tags, it copies the right files without actually cloning the repo. (You can modify this behavior with --prefer-source and --prefer-dist, see install options.)

## Examples
```
"require": {
    "vendor/package": "1.3.2", // exactly 1.3.2

    // >, <, >=, <= | specify upper / lower bounds
    "vendor/package": ">=1.3.2", // anything above or equal to 1.3.2
    "vendor/package": "<1.3.2", // anything below 1.3.2

    // * | wildcard
    "vendor/package": "1.3.*", // >=1.3.0 <1.4.0

    // ~ | allows last digit specified to go up
    "vendor/package": "~1.3.2", // >=1.3.2 <1.4.0
    "vendor/package": "~1.3", // >=1.3.0 <2.0.0

    // ^ | doesn't allow breaking changes (major version fixed - following semver)
    "vendor/package": "^1.3.2", // >=1.3.2 <2.0.0
    "vendor/package": "^0.3.2", // >=0.3.2 <0.4.0 // except if major version is 0
}
```


|Constraint|Internally|
| :------ | ------: |
| `1.2.3` | `=1.2.3.0-stable` |
|`>1.2` | `>1.2.0.0-stable`|
|`>=1.2` | `>=1.2.0.0-dev`|
|`>=1.2-stable` | `>=1.2.0.0-stable`|
|`<1.3`   | `<1.3.0.0-dev`|
|`<=1.3`  | `<=1.3.0.0-stable`|
|`1 - 2`  | `>=1.0.0.0-dev <3.0.0.0-dev`|
|`~1.3`   | `>=1.3.0.0-dev <2.0.0.0-dev`|
|`^1.3`   | `>=1.3.0.0-dev <2.0.0.0-dev`|
|`~1.3.0` | `>=1.3.0.0-dev <1.4.0.0-dev`|
|`^1.3.0` | `>=1.3.0.0-dev <2.0.0.0-dev`|
|`1.4.*`  | `>=1.4.0.0-dev <1.5.0.0-dev`|

[https://semver.mwl.be/#?package=monolog%2Fmonolog&version=~1&minimum-stability=dev](https://semver.mwl.be/#?package=monolog%2Fmonolog&version=~1&minimum-stability=dev)

`minimum-stability` 配置项定义了包在选择版本时对稳定性的选择的默认行为。默认是`stable`。它的值如下（按照稳定性升序排序）：`dev`，`alpha`，`beta`，`RC` 和 `stable` 。除了修改这个配置去修改这个默认行为，我们还可以通过稳定性标识（例如 `@stable` 和 `@dev` ）来安装一个相比于默认配置不同稳定性的版本。 

reference: [https://getcomposer.org/doc/](https://getcomposer.org/doc/)
