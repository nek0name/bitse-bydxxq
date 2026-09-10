# 管理端连接与运行

管理端和 APK 共用 `ApiClient`，默认连接当前联调服务器：

`https://www.u910784.nyat.app:40004`

登录页“服务地址”可直接修改；运行时环境变量 `CHARGING_SERVER_URL` 优先于构建默认值。例如临时连接本地服务：

```bash
CHARGING_SERVER_URL=http://127.0.0.1:8080 ./build/admin/apps/admin-app/charging-admin
```

首次构建（需要 Qt 6.2 及以上的 Widgets、Charts、Network、DBus 开发组件）：

```bash
cmake -S . -B build/admin -G Ninja -DBUILD_USER_APP=OFF -DBUILD_SERVER=OFF -DBUILD_TESTING=OFF
cmake --build build/admin --target charging-admin
./build/admin/apps/admin-app/charging-admin
```

已有构建目录会保留 CMake 缓存中的旧地址，可显式更新后重新构建：

```bash
cmake -S . -B build/admin -DCHARGING_DEFAULT_SERVER_URL=https://www.u910784.nyat.app:40004
cmake --build build/admin --target charging-admin
```

管理端操作与 APK 使用同一套服务器数据；请使用分配的管理员账号登录。
