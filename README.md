<a href="https://github.com/JanSeliv/PoolManager/blob/main/LICENSE">![License](https://img.shields.io/badge/license-MIT-brightgreen.svg)</a>
<a href="https://www.unrealengine.com/">![Unreal Engine](https://img.shields.io/badge/Unreal-5.4-dea309?style=flat&logo=unrealengine)
</a>

<br/>
<p align="center">
<a href="https://github.com/JanSeliv/PoolManager">
<img src="https://github.com/JanSeliv/PoolManager/blob/main/Resources/Icon128.png?raw=true" alt="Logo" width="80" height="80">
</a>
<h3 align="center">🔄 Pool Manager</h3>
<p align="center">
Reuse any objects and actors to improve performance
<br/>
<br/>
<a href="https://discord.gg/jbWgwDefnE"><strong>Join our Discord ››</strong></a>
<br/>
<a href="https://github.com/JanSeliv/PoolManager/releases">Releases</a>
·
<a href="https://docs.google.com/document/d/1YxbIdc9lZRl5ozI7_1LTBfdzJWTwhxwz2RKA-r0Q4po">Docs</a>
</p>

## 🌟 About

The Pool Manager helps reuse objects that show up often, instead of creating and destroying them each time.

Creating and destroying objects, like projectiles or explosions, can be slow and cause issues such as making the game slow or laggy when done frequently.

The Pool Manager alleviates these problems by maintaining a pool of objects. Instead of creating and destroying objects all the time, the Pool Manager keeps these objects for reuse. This strategy improves the smoothness of the game.

![PoolManager](https://github.com/JanSeliv/PoolManager/assets/20540872/0af55b33-732c-435d-a5b3-2d7e36cdebf2)

## 🎓 Sample Projects

Check out our [Release](https://github.com/JanSeliv/PoolManager/releases) page for two sample projects showcasing the Pool Manager, one with blueprints and another in C++.

Also, explore this [game project repository](https://github.com/JanSeliv/Bomber) to view the Pool Manager in action.

## 📅 Changelog
#### 2025-01-01
- Updated to **Unreal Engine 5.5**.
- Implemented **Priorities** to process important objects faster than others:
>![Priorities](https://github.com/user-attachments/assets/3a0501f6-28ed-4bc9-8f06-93bf95385625)
- Implemented **Pool Object Callback** interface by [Bigotry0](https://github.com/Bigotry0) to notify objects when they are taken from or returned:
>![Interface](https://github.com/user-attachments/assets/8cd05e9b-3877-43ed-9285-50a56641132f)
#### 2024-05-05
- Updated to **Unreal Engine 5.4**.
- Implemented **User Widgets support** allowing to pool widgets.
> ![UserWidgets](https://github.com/JanSeliv/PoolManager/assets/20540872/53652ec5-9795-4473-bbe6-792ad2574bc2)
- Added [Take From Pool Array](https://docs.google.com/document/d/1YxbIdc9lZRl5ozI7_1LTBfdzJWTwhxwz2RKA-r0Q4po/edit#heading=h.m317652zeuu9) and [Return To Pool Array](https://docs.google.com/document/d/1YxbIdc9lZRl5ozI7_1LTBfdzJWTwhxwz2RKA-r0Q4po/edit#heading=h.la64v3qmhabw) functions to support pooling multiple objects:
> ![PoolManagerArray](https://github.com/JanSeliv/PoolManager/assets/20540872/b28b45ec-5ce0-48ac-ad7e-33e6bcb7758e)
#### 2023-11-25
- Updated to **Unreal Engine 5.3**.
- Introduced **Factories** to handle differences in pools by object archetypes (e.g.: uobjects, actors, components, widgets etc.).
- **Take From Pool** now spreads out the creation of large pools of UObjects and Actors over multiple frames to avoid any hitches.
> ![image](https://github.com/JanSeliv/PoolManager/assets/20540872/10bdf24f-d078-4dd8-96bf-de5d92421bc8)
#### 2023-05-28
- 🎉 Initial public release on Unreal Engine 5.2

## 📫 Feedback & Contribution

Feedback and contributions from the community are highly appreciated!

If you'd like to contribute, please fork the project and create a pull request targeting the `develop` branch.

If you've found a bug or have an idea for a new feature, please open a new issue on GitHub. Thank you!

## 📜 License

This project is licensed under the terms of the MIT license. See [LICENSE](LICENSE) for more details.

We hope you find this plugin useful and we look forward to your feedback and contributions.
