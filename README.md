# Avestan
a fork of Avesta ([http://lamoo.s53.xrea.com/](http://lamoo.s53.xrea.com/))

タブ型ファイラ Avesta 0.3.1.9 (avesta-0.3.1.9src.zip) を Visual Studio Community 2026 で
ビルドできるようにしたもの。

## 実行ファイルのダウンロード
上の [release](https://github.com/takechi/Avestan/releases) にあります。


## ソースからビルド
### 必要なもの
* cmake
* Visual Studio Community 2026
* MSXML 6.0

### ビルド方法
* my_cmake.bat を実行。
* build/Avestan.slnx を開いて F5 でビルドできるはず。

<details>
<summary>32bit (x86) 版のビルド</summary>

* my_cmake_Win32.bat を実行。
* build_Win32/Avestan.slnx を開いて F5 でビルドできるはず。

</details>

### スクリプト拡張
python によるスクリプト拡張を利用する場合は、

* Python 3.11 以上のインストール
* python3.dll があるディレクトリを PATH に登録

が必要。

## ライセンス
* [Avesta License : version 1.0](https://github.com/takechi/Avestan/blob/master/licence.txt)
* 変更者は [TAKECHI Kohei](https://github.com/takechi/)。
* 変更内容は https://github.com/takechi/Avestan/commits/master の通り。
