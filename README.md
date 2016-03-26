# Avestan
a fork of Avesta ( http://lamoo.s53.xrea.com/ )

タブ型ファイラ Avesta 0.3.1.9 (avesta-0.3.1.9src.zip) を Visual Studio Community 2015 で
ビルドできるようにしたもの。


## ビルドに必要なもの
* Visual Studio Community 2015
  * VS Express だと、ATL がないので（そのままでは）ビルドできない。
* MSXML 4.0
* Python 3.5 (32bit)

## ビルド方法
* pygmy プロジェクトで Python.h のインクルードパス、python35.lib のライブラリパスだけ変更すればビルドできるはず。
  * Python 3.5 (32bit) を C:\Python\Python35-32 にインストールしている場合はこの変更も不要。

## 注意
* とりあえずコンパイルが通り、エラーなく起動できるようにしただけ。
* コンパイルのためにコードをコメントアウトしたりしているので、一部機能（フォルダツリーなど）が使えない。
* **どんなバグがあるか分からないので、ご利用は自己責任で。**
