# Avestan
a fork of Avesta ( http://lamoo.s53.xrea.com/ )

タブ型ファイラ Avesta 0.3.1.9 (avesta-0.3.1.9src.zip) を Visual Studio Community 2015 で
ビルドできるようにしたもの。


## ビルドに必要なもの
* Visual Studio Community 2015
  * VS Express だと、ATL がないので（そのままでは）ビルドできない。
* MSXML 4.0
* Python 3.x
 * 検証環境は Python 3.5

## ビルド方法
* pygmy プロジェクトで Python.h のインクルードパス、python3x.lib のライブラリパスだけ変更すればビルドできるはず。
  * Python 3.5 (32|64)bit を C:\Python\Python35-(32|64) にインストールしている場合はこの変更も不要。

## 注意
* とりあえずコンパイルが通り、エラーなく起動できるようにしただけ。
* **どんなバグがあるか分からないので、ご利用は自己責任で。**

### ライセンス
* Avesta License : version 1.0 ( https://github.com/takechi/Avestan/blob/master/licence.txt )。
* 変更者は TAKECHI Kohei ( https://github.com/takechi/ )。
* 変更内容は https://github.com/takechi/Avestan/commits/master の通り。
