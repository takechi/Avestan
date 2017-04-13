# Avestan
a fork of Avesta ( http://lamoo.s53.xrea.com/ )

タブ型ファイラ Avesta 0.3.1.9 (avesta-0.3.1.9src.zip) を Visual Studio Community 2017 で
ビルドできるようにしたもの。

## 実行ファイル
上の [release](https://github.com/takechi/Avestan/releases) にあります。


## ビルド
### 必要なもの
* Visual Studio Community 2017
  * VS Express だと、ATL がないので（そのままでは）ビルドできない。
* MSXML 4.0

### ビルド方法
* src/Avesta.sln を開いて F5 でビルドできるはず。

### スクリプト拡張
python によるスクリプト拡張を利用する場合は、Python 3.6 のインストールと、
sln 内の pygmy プロジェクトのビルドが必要。

## ToDo
* キーボード操作で空のディレクトリに移動すると ListView からフォーカスが外れる現象の対応。

## ライセンス
* Avesta License : version 1.0 ( https://github.com/takechi/Avestan/blob/master/licence.txt )。
* 変更者は TAKECHI Kohei ( https://github.com/takechi/ )。
* 変更内容は https://github.com/takechi/Avestan/commits/master の通り。
