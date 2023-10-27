# 🔄 Pool Manager

The Pool Manager helps reuse objects that show up often, instead of creating and destroying them each time.

Creating and destroying objects, like projectiles or explosions, can be slow and cause issues such as making the game slow or laggy when done frequently.

The Pool Manager alleviates these problems by maintaining a pool of objects. Instead of creating and destroying objects all the time, the Pool Manager keeps these objects for reuse. This strategy improves the smoothness of the game.

![PoolManager](https://github.com/JanSeliv/PoolManager/assets/20540872/0af55b33-732c-435d-a5b3-2d7e36cdebf2)

## 📚 Documentation

Detailed documentation about the Pool Manager can be found [here](https://docs.google.com/document/d/1YxbIdc9lZRl5ozI7_1LTBfdzJWTwhxwz2RKA-r0Q4po).

## 🎓 Sample Projects

Check out our [Release](https://github.com/JanSeliv/PoolManager/releases) page for two sample projects showcasing the Pool Manager, one with blueprints and another in C++.

Also, explore this [game project repository](https://github.com/JanSeliv/Bomber) to view the Pool Manager in action.

## 📅 Changelog
####
- Updated to **Unreal Engine 5.3**.
- Introduced **Factories** to handle differences in pools by object archetypes (e.g.: uobjects, actors, components, widgets etc.).
- **Take From Pool** now spreads out the creation of large pools of UObjects and Actors over multiple frames to avoid any hitches.
  ![image](https://github.com/JanSeliv/PoolManager/assets/20540872/10bdf24f-d078-4dd8-96bf-de5d92421bc8)
#### 2023-05-28
- 🎉 Initial public release on Unreal Engine 5.2

## 📫 Feedback & Contribution

Feedback and contributions from the community are highly appreciated!

If you'd like to contribute, please fork the project and create a pull request targeting the `develop` branch.

If you've found a bug or have an idea for a new feature, please open a new issue on GitHub. Thank you!

## 📜 License

This project is licensed under the terms of the MIT license. See [LICENSE](LICENSE) for more details.

We hope you find this plugin useful and we look forward to your feedback and contributions.
