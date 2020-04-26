= Triangulation by Ear Clipping

== はじめに

本章では、多角形の三角形分割の手法のひとつである、「耳刈り取り法（Ear Clipping法）、以下耳刈り取り法」について説明します。
通常の単純多角形の三角形分割だけでなく、穴のある多角形や階層構造になっている多角形の三角形分割についても説明します。

本章のサンプルは@<br>{}
@<href>{https://github.com/IndieVisualLab/UnityGraphicsProgramming4}@<br>{}
の「TriangulationByEarClipping」です。

=== サンプルの操作方法

サンプルのDrawTestシーンを実行します。
GameViewを左クリックすると画面上に点を打ちます。続けて他の地点を左クリックすると最初の点と線で結びます。繰り返していくと多角形ができます。線を引く時、線が交差しないように注意してください。
右クリックで多角形を三角形分割してメッシュを生成します。生成したメッシュの中で多角形を生成すると、穴のある多角形ができます。

#@# メインビジュアル
//image[kaiware/sample_ss001][サンプルを実行した画面][scale=0.5]

== 単純多角形の三角形分割

単純多角形とは、自己の線分で交差しない閉じた多角形の事を言います。

#@# 単純多角形と非単純多角形の図
//image[kaiware/simple_polygon001][左：単純多角形、右：非単純多角形][scale=0.5]

どんな単純多角形も三角形分割が可能です。n個の頂点をもつ単純多角形を三角形分割すると、n-2個の三角形ができます。

== 耳刈り取り法（EarClipping法）

多角形の三角形分割の手法はいくつも存在しますが、今回は実装がシンプルな「耳刈り取り法」について説明します。
「耳刈り取り法」は、「Two ears theorem」という定理に基づいて分割します。この「Ear(耳)」というのは、「2辺が多角形の辺であり、残り1辺が多角形の内部に存在する三角形」のことを指し、この定理は、「4本以上の辺を持つ穴のない単純多角形は少なくとも２つの耳を持っている」という定理です。

#@# 耳（四角形と２つの耳）
//image[kaiware/ear001][耳][scale=0.5]

「耳刈り取り法」は、この「耳」三角形を探し、多角形から取り除いていくアルゴリズムです。この「耳刈り取り法」ですが、他の分割アルゴリズムに比べてシンプルな半面、処理が遅いので速度が求められる場面ではあまり使えないと思います。

=== 三角形分割の流れ

まず、与えられた多角形の頂点配列の中から「耳」を探します。「耳」の条件は以下の2点です。

 * 多角形の頂点viの前後の頂点(vi-1,vi+1)との線分のなす角度(内角)が180度以内（凸頂点という）
 * 多角形の頂点vi-1, vi, vi+1からなる三角形の中に他の頂点が含まれていない

#@# 180度以内、三角形の中に他の頂点が含まれていないの図
//image[kaiware/ear_check001][耳の条件（180度以内、三角形の中に他の頂点が含まれない）][scale=0.5]

上記の条件を満たした頂点viを耳リストに追加します。この処理はサンプルの Triangulation.cs の InitializeVertices関数が該当します。
そして、耳リストの先頭から耳を構成する三角形を作成し、頂点viを頂点配列から取り除きます。@<br>{}
頂点viを取り除くと、多角形の形が変わります。残された頂点vi-1,vi+1に対して、再度前述の耳判定を行います。頂点vi-1,vi+1が耳の条件を満たしていれば、耳リストの末尾に追加されますが、逆に耳リストから削除される場合もあります。この処理はサンプルの Triangulation.cs の CheckVertex関数と、EarClipping関数が該当します。

#@# 頂点viを削除する前の多角形と、削除したあとの多角形
//image[kaiware/ear_check002][頂点viを削除する前の多角形と、削除したあとの多角形][scale=0.75]

ある単純多角形を例に一連の流れを図で表してみます。

#@# ある多角形
//image[kaiware/triangulation0][ある単純多角形][scale=0.5]

まず、耳を探します。この場合、耳リストには頂点0,1,4,6が含まれます。頂点2,5は凸頂点ではないので除外、頂点3は三角形2,3,4の中に頂点5が含まれているので除外されます。
@<br>{}
まず、耳リストの先頭の頂点0を取り出します。頂点0の前後の頂点1,6で三角形を作ります。頂点0を頂点配列から削除し、前後の頂点1,6を結んで新しい多角形にします。そして頂点1,6について耳判定をします。もともと2つとも耳でしたが、耳判定後も耳のままです。この時の耳リストは、1,4,6です。

#@# 頂点0を削除した多角形
//image[kaiware/triangulation1][頂点0を削除した多角形][scale=0.5]

次に、耳リストの先頭から頂点1を取り出します。頂点1の前後の頂点2,6で三角形を作ります。頂点1を頂点配列から削除し、前後の頂点2,6を結んで新しい多角形にします。そして頂点2,6について耳判定をします。頂点1がなくなったことで、頂点2が凸頂点になり、耳の条件を満たしましたので、耳リストに追加します。頂点6は耳のままです。この時の耳リストは、4,6,2です。

#@# 頂点1を削除した多角形
//image[kaiware/triangulation2][頂点1を削除した多角形][scale=0.5]

次に、耳リストの先頭から頂点4を取り出します。頂点4の前後の頂点3,5で三角形を作ります。頂点4を頂点配列から削除し、前後の頂点3,5を結んで新しい多角形にします。そして頂点3,5について耳判定をします。頂点4がなくなったことで、頂点3の前後の頂点2,5で作られる三角形の中に他の頂点が含まれなくなったので、頂点3を耳リストに追加します。また、頂点5の内角が180度以下になったことで凸頂点になり、耳の条件を満たしましたので、耳リストに追加します。この時の耳リストは、6,2,3,5 です。

#@# 頂点4を削除した多角形
//image[kaiware/triangulation3][頂点4を削除した多角形][scale=0.5]

次に、耳リストの先頭から頂点6を取り出します。頂点6の前後の頂点2,5で三角形を作ります。頂点6を頂点配列から削除し、前後の頂点2,5を結んで新しい多角形にします。そして頂点2,5について耳判定をします。もともと2つとも耳でしたが、耳判定後も耳のままです。この時の耳リストは、2,3,5 です。

#@# 頂点6を削除した多角形
//image[kaiware/triangulation4][頂点6を削除した多角形][scale=0.5]

次に、耳リストの先頭から頂点2を取り出…そうと思いましたが、残された多角形の頂点が3つしか無いのでこのまま三角形にして三角形分割は終了です。
最終的な三角形分割の結果は次のとおりです。

#@# 三角形分割の最終的な結果
//image[kaiware/triangulation5][三角形分割の結果][scale=0.5]


== 穴空き多角形の三角形分割

次に、穴のある多角形の三角形分割について解説します。もともと「耳刈り取り法」は穴のある多角形には適用できませんが、図のように外側の多角形に切れ込みをいれて内側の多角形と繋げてしまえば、内側の多角形が外側の多角形の一部になり、耳刈り取り法が適用できるようになります。この方法は複数の穴がある多角形でも可能です。

#@# 内外多角形の結合
//image[kaiware/combine_polygon001][内外多角形の結合（図はかなり大げさな表現）][scale=0.5]

=== 外側の多角形と内側の多角形の結合の流れ

前提として、外側の多角形と内側の多角形の頂点の順序は逆にする必要があります。たとえば、外側の多角形が時計回りに頂点が並んでいる場合、内側の多角形は反時計回りに並んでいる必要があります。
次の多角形を例に結合の流れを説明します。

#@# 外側の多角形と内側の多角形
//image[kaiware/polygon_with_hole001][穴のある多角形][scale=0.5]

　1. 複数の穴（内側の多角形）がある場合、内側の多角形の中で最もX座標が大きい（右側にある）多角形とその頂点を探します。

#@# 最もX座標が大きい内側の多角形を探す
//image[kaiware/polygon_with_hole002][最もX座標が大きい頂点][scale=0.5]

　2. 最もX座標の大きい頂点をMとします。Mから右にまっすぐ線を引きます。

#@# Mから右に線を引く
//image[kaiware/polygon_with_hole003][頂点Mから右に線を引く][scale=0.5]

　3. 頂点Mから右に伸ばした線と交差する外側の多角形の辺と交点Iを探します。複数の辺と交差する場合、最も頂点Mに近い交点の辺を選択します。

#@# Mと交差する辺と交点I
//image[kaiware/polygon_with_hole003_1][頂点Mと交点I][scale=0.5]

　4. 交差する辺の頂点のうち、最もX座標が大きい頂点Pを選択します。頂点M,I,Pを結ぶ三角形の中に他の頂点が含まれないかチェックします。

#@# 三角形M,I,P
//image[kaiware/polygon_with_hole004][三角形M,I,P][scale=0.5]

　5. 三角形M,I,Pに他の頂点が含まれない場合は、分割が可能なので、外側の多角形の頂点Pから内側の多角形の頂点Mを繋げ、内側の多角形を反時計回りに一周します。再度Mから、外側の多角形の頂点Pに繋げる時、頂点Mと頂点Pを複製して別の頂点とします(頂点M',P')。入る線と出る線を分けることで、見かけ上は線が重なっていますが、頂点の順序としては交差していない、ひとつの単純多角形となります。

#@# 外側の多角形と内側の多角形を繋げた図
//image[kaiware/polygon_with_hole005][外側の多角形と内側の多角形を繋げた図][scale=0.5]

　6. 三角形M,I,Pの中に他の頂点Rが含まれていた場合、その頂点Rを選択しますが、複数の頂点が含まれている場合、線分M,Iと線分M,Rのなす角θが最も小さい頂点Rを選択して、5の処理を行います。

#@# 三角形M,I,Pに他の頂点Rが含まれていた場合、線分MI,MRのなす角が最も小さい頂点Rを選択する図
//image[kaiware/polygon_with_hole006][線分MI,MRのなす角θが最も小さい頂点R][scale=0.5]

　7. 1に戻って他の内側の多角形と結合していきます。

== 入れ子構造の多角形の三角形分割

次に、入れ子構造の多角形の三角形分割について説明します。
穴のある多角形の結合処理と三角形分割処理は前項で説明したので、ここでは主に多角形の親子関係のツリー構築の手順について説明します。

 1. 多角形を矩形領域としたときの面積が大きい順にソートします。多角形の最小/最大座標の頂点で作る矩形領域の面積です。
 2. 面積が大きい多角形の中に、他の多角形が全頂点が含まれているかを再帰的に判定し、親子関係のツリーを作ります。この時、最上位のルートは空の多角形（いわゆるダミー）として、あとの多角形の結合処理には使いません。何故、最上位をダミーにするのかというと、複数の全く被らない多角形の集合が渡された場合に、最上位が1つにならないからです。ダミーの最上位の下に全く重ならない複数の多角形をぶら下げることで、処理が単純にできます。また、階層が偶数階の時は、内側の多角形になるので、該当多角形の頂点配列を反時計回りに並べ替えます。
 3. 親子関係ツリーができたら、上から1つ多角形を取り出します。それが外側の多角形になります。
 4. 外側の多角形の1階層下(子)の多角形を取り出します。そして、内側の多角形として外側の多角形と結合して、三角形分割を行います。子がない場合はそのまま三角形分割します。
 5. 3に戻って結合と分割を繰り返します。4で外側の多角形と内側の多角形群が１つの組み合わせとして処理されるので、次に取り出す三角形はまた外側の多角形になります。

次の多角形の集合を例として説明します。

#@# 入れ子構造の多角形
//image[kaiware/nested_polygons001][入れ子構造の多角形][scale=0.5]

多角形の親子関係を作ると次のようなツリーになりました。

#@# 入れ子構造の多角形と親子関係のツリー構造 1,2,4,3,5
//image[kaiware/nested_polygons002][左：入れ子構造の多角形 右：親子関係][scale=0.5]

ツリーの最上位（ダミーは除く）の多角形1と、その子の多角形2,4を取り出します。

#@# 最上位の多角形1を取り出し、その子2,4も取り出す
//image[kaiware/nested_polygons003][多角形1,2,4を取り出す][scale=0.5]

多角形1,2,4を右から順に結合していきます。

#@# 多角形1,2,4を結合
//image[kaiware/nested_polygons004][多角形1,2,4を結合][scale=0.5]

結合した多角形を三角形分割します。

#@# 多角形1,2,4を三角形分割
//image[kaiware/nested_polygons005][結合した多角形を三角形分割][scale=0.5]

三角形分割した多角形を取り除きます。親子関係ツリーの残りは3と5です。まず、3から取り出します。

#@# 残りの多角形3を取り出す
//image[kaiware/nested_polygons006][多角形3][scale=0.5]

//embed[latex]{
\clearpage
//}

3には子は無いのでそのまま三角形分割します。

#@# 多角形3を三角形分割した図
//image[kaiware/nested_polygons007][多角形3を三角形分割][scale=0.5]

三角形分割した多角形を取り除きます。親子関係ツリーの残りは5だけです。5は三角形で子がないので、そのまま終了します。これで入れ子構造の多角形の三角形分割は完了です。

#@# 多角形5を三角形分割？した図
//image[kaiware/nested_polygons008][最後の多角形5][scale=0.5]

== 実装

今まで説明した３つのアルゴリズムを全て実装したサンプルのソースコードの説明に移ります。

=== Polygonクラス
まず、多角形の頂点配列を管理するPolygonクラスを定義します。
Polygonクラスは、頂点座標の配列やループの方向などの情報の保持、多角形の中に多角形が入っているかの判定を行います。

//emlist[Polygon.cs]{
public class Polygon
{
    // ループの方向
    public enum LoopType
    {
        CW,     // 時計回り
        CCW,    // 反時計回り
        ERR,    // 不定（向きがない）
    }

    public Vector3[] vertices;  // 頂点配列
    public LoopType loopType;   // ループの方向

    //～省略～
}
//}

=== Triangulationクラス

実際に多角形の三角形分割を行うTriangulationクラスです。TriangulationクラスのTriangulate関数がメインです。

==== 三角形分割に使うデータ構造

//emlist[Triangulation.csのデータ構造定義]{
// 頂点配列
List<Vector3> vertices = new List<Vector3>();

// 頂点番号のリスト（末尾と先頭がつながってることにする）
LinkedList<int> indices = new LinkedList<int>();

// 耳頂点リスト
List<int> earTipList = new List<int>();
//}

処理対象の多角形の頂点座標の配列を格納するvertices、多角形の頂点の番号（インデックス）を格納するindices、耳を格納するearTipListを定義しています。
indicesは前後の頂点を参照する必要があるため、双方向リストの性質があるLinkedListを使っています。

==== 階層構造を作る

まず、外から多角形を構成する頂点配列を渡されたら、それをPolygonクラスとしてリストに格納します。

//emlist[Polygonリスト]{
// 多角形リスト
List<Polygon> polygonList = new List<Polygon>();

public void AddPolygon(Polygon polygon)
{
    polygonList.Add(polygon);
}
//}

Triangulate関数の冒頭で、多角形データを追加したPolygonリストを矩形領域の面積の大きい順にソートします。

//emlist[Polygonリストのソート部分]{
// 多角形リストの矩形領域の面積の大きい順にソート
polygonList.Sort((a, b) => Mathf.FloorToInt(
  (b.rect.width * b.rect.height) - (a.rect.width * a.rect.height)
  ));
//}

次に、ソートしたPolygonリストをツリー構造を作るTreeNodeクラスに詰め込んでいきます。

//emlist[PolygonリストをTreeNodeに詰める部分]{
// ルート作成（空っぽ）
polygonTree = new TreeNode<Polygon>();

// 多角形の階層構造を作る
foreach (Polygon polygon in polygonList)
{
   TreeNode<Polygon> tree = polygonTree;

   CheckInPolygonTree(tree, polygon, 1);
}
//}

TreeNodeは以下のようになっています。よくあるツリー構造だと思いますが、空の最上位ノードのために、中身が存在するかのフラグisValueを定義しています。

//emlist[TreeNode.cs]{
public class TreeNode<T>
{
    public TreeNode<T> parent = null;
    public List<TreeNode<T>> children = new List<TreeNode<T>>();

    public T Value;
    public bool isValue = false;

    public TreeNode(T val)
    {
        Value = val;
        isValue = true;
    }

    public TreeNode()
    {
        isValue = false;
    }

    public void AddChild(T val)
    {
        AddChild(new TreeNode<T>(val));
    }

    public void AddChild(TreeNode<T> tree)
    {
        children.Add(tree);
        tree.parent = this;
    }

    public void RemoveChild(TreeNode<T> tree)
    {
        if (children.Contains(tree))
        {
            children.Remove(tree);
            tree.parent = null;
        }
    }
}
//}

Triangulation.csに戻り、多角形の階層構造を作るCheckInPolygonTree関数の中身です。
自身の多角形の中に渡された多角形が入るか確認し、更に自身の子の中にも入るかを再帰的に判定しています。
自身には含まれるが、子には含まれない、または子が存在しない場合に、渡された多角形を自身の子にします。

//emlist[CheckInPolygonTree関数]{
bool CheckInPolygonTree(TreeNode<Polygon> tree, Polygon polygon, int lv)
{
    // 自身に多角形が存在するか？
    bool isInChild = false;
    if (tree.isValue)
    {
        if (tree.Value.IsPointInPolygon(polygon))
        {
            // 自身に含まれる場合、子にも含まれるか検索
            for(int i = 0; i < tree.children.Count; i++)
            {
                isInChild |= CheckInPolygonTree(
                    tree.children[i], polygon, lv + 1);
            }

            // 子に含まれない場合は自身の子にする
            if (!isInChild)
            {
                // 必要であれば頂点の順番を反転する
                // 偶数ネストの時はInnerなのでCW
                // 奇数ネストの時はOuterなのでCCW
                if (
                    ((lv % 2 == 0) &&
                     (polygon.loopType == Polygon.LoopType.CW)) ||
                    ((lv % 2 == 1) &&
                     (polygon.loopType == Polygon.LoopType.CCW))
                    )
                {
                    polygon.ReverseIndices();
                }

                tree.children.Add(new TreeNode<Polygon>(polygon));
                return true;
            }
        }
        else
        {
            // 含まれない
            return false;
        }
    }
    else
    {
        // 自身に値がない場合、子の方だけ検索
        for (int i = 0; i < tree.children.Count; i++)
        {
            isInChild |= CheckInPolygonTree(
                tree.children[i], polygon, lv + 1);
        }

        // 子に含まれない場合は自身の子にする
        if (!isInChild)
        {
            // 必要であれば頂点の順番を反転する
            // 偶数ネストの時はInnerなのでCW
            // 奇数ネストの時はOuterなのでCCW
            if (
                ((lv % 2 == 0) &&
                 (polygon.loopType == Polygon.LoopType.CW)) ||
                ((lv % 2 == 1) &&
                 (polygon.loopType == Polygon.LoopType.CCW))
                )
            {
                polygon.ReverseIndices();
            }
            tree.children.Add(new TreeNode<Polygon>(polygon));
            return true;
        }
    }

    return isInChild;
}
//}

==== 穴のある多角形の処理（内外多角形の結合）

複数の内側の多角形がある場合、内側の多角形の中で最もX座標が大きい頂点とその多角形を選択します。その時、判定用にX座標と頂点番号、多角形の番号の情報をまとめておくクラスを定義します。

//embed[latex]{
\clearpage
//}

//emlist[XMaxData構造体]{
/// <summary>
/// X座標最大値と多角形の情報
/// </summary>
struct XMaxData
{
    public float xmax;  // x座標最大値
    public int no;      // 多角形の番号
    public int index;   // xmaxの頂点番号

    public XMaxData(float x, int n, int ind)
    {
        xmax = x;
        no = n;
        index = ind;
    }
}
//}

次に、実際の結合処理ですが、複数の多角形をX座標が大きい順にソートする処理と、結合する処理の２つに分かれています。
まずは、複数の多角形をX座標が大きい順にソートする処理です。

//emlist[CombineOuterAndInners関数]{
Vector3[] CombineOuterAndInners(Vector3[] outer, List<Polygon> inners)
{
    List<XMaxData> pairs = new List<XMaxData>();

    // 内側の多角形の中で最もX座標が大きい頂点を持つものを探す
    for (int i = 0; i < inners.Count; i++)
    {
        float xmax = inners[i].vertices[0].x;
        int len = inners[i].vertices.Length;
        int xmaxIndex = 0;
        for (int j = 1; j < len; j++)
        {
            float x = inners[i].vertices[j].x;
            if(x > xmax)
            {
                xmax = x;
                xmaxIndex = j;
            }
        }
        pairs.Add(new XMaxData(xmax, i, xmaxIndex));
    }

    // 右順(xmaxが大きい順)にソート
    pairs.Sort((a, b) => Mathf.FloorToInt(b.xmax - a.xmax));

    // 右から順に結合
    for (int i = 0; i < pairs.Count; i++)
    {
        outer = CombinePolygon(outer, inners[pairs[i].no], pairs[i].index);
    }

    return outer;
}
//}

次に結合処理の部分です。
CombinePolygon関数の中で、内側の多角形の最もX座標が大きい頂点Mから横に線を引き、その線と交差する外側の多角形の線分を探します。

//emlist[CombinePolygon関数の序盤]{
Vector3[] CombinePolygon(Vector3[] outer, Polygon inner, int xmaxIndex)
{
    Vector3 M = inner.vertices[xmaxIndex];

    // 交点を探す
    Vector3 intersectionPoint = Vector3.zero;
    int index0 = 0;
    int index1 = 0;

    if (GeomUtil.GetIntersectionPoint(M,
      new Vector3(maxX + 0.1f, M.y, M.z),
      outer, ref intersectionPoint,
      ref index0, ref index1))
    {
      ～省略～
//}

線分M,Iと外側の多角形の線分の交点を探す関数、GeometryUtil.GetIntersectionPointは以下のようになっています。
ポイントは、外側の多角形は時計回りなので、交差する線分の始点が線分M,Iより上で終点が下になるものだけを探すようにしている点です。
そうすることで、すでに結合した内外多角形で、外側の多角形から内側の多角形に接続した線分を選択してしまうと、頂点の順序がおかしくなってしまうのを防ぎます。

//emlist[GetIntersectionPoint関数]{
public static bool GetIntersectionPoint(Vector3 start, Vector3 end,
                                        Vector3[] vertices,
                                        ref Vector3 intersectionP,
                                        ref int index0, ref int index1)
{
    float distanceMin = float.MaxValue;
    bool isHit = false;

    for(int i = 0; i < vertices.Length; i++)
    {
        int index = i;
        int next = (i + 1) % vertices.Length;        // 次の頂点

        Vector3 iP = Vector3.zero;
        Vector3 vstart = vertices[index];
        Vector3 vend = vertices[next];

        // 交差する多角形の線分の始点が線分M,I以上にいること
        Vector3 diff0 = vstart - start;
        if (diff0.y < 0f)
        {
            continue;
        }

        // 交差する多角形の線分の終点が線分M,I以下にいること
        Vector3 diff1 = vend - start;
        if (diff1.y > 0f)
        {
            continue;
        }

        if (IsIntersectLine(start, end, vstart, vend, ref iP))
        {
            float distance = Vector3.Distance(start, iP);

            if (distanceMin >= distance)
            {
                distanceMin = distance;
                index0 = index;
                index1 = next;
                intersectionP = iP;
                isHit = true;
            }
        }

    }

    return isHit;
}
//}

交点を見つけた後は、交差した線分の一番X座標が大きい頂点、頂点M、交点Iから作る三角形に他の頂点が含まれてないか確認します。
三角形の中に頂点が含まれているかの判定には、二次元の外積を使って頂点が三角形の線分のどちら側にいるかを出しています。頂点がすべての線の右側にいれば三角形の中に含まれていることになります。

//emlist[GeometryUtil.csのIsTriangle関数とCheckLine関数]{
/// <summary>
/// 線と頂点の位置関係を返す
/// </summary>
/// <param name="o"></param>
/// <param name="p1"></param>
/// <param name="p2"></param>
/// <returns> +1 : 線の右 -1 : 線の左 0 : 線上</returns>
public static int CheckLine(Vector3 o, Vector3 p1, Vector3 p2)
{
    double x0 = o.x - p1.x;
    double y0 = o.y - p1.y;
    double x1 = p2.x - p1.x;
    double y1 = p2.y - p1.y;

    double x0y1 = x0 * y1;
    double x1y0 = x1 * y0;
    double det = x0y1 - x1y0;

    return (det > 0D ? +1 : (det < 0D ? -1 : 0));
}

/// <summary>
/// 三角形(時計回り)と点の内外判定
/// </summary>
/// <param name="o"></param>
/// <param name="p1"></param>
/// <param name="p2"></param>
/// <param name="p3"></param>
/// <returns> +1 : 外側 -1 : 内側 0 : 線上</returns>
public static int IsInTriangle(Vector3 o,
                               Vector3 p1,
                               Vector3 p2,
                               Vector3 p3)
{
    int sign1 = CheckLine(o, p2, p3);
    if (sign1 < 0)
    {
        return +1;
    }

    int sign2 = CheckLine(o, p3, p1);
    if (sign2 < 0)
    {
        return +1;
    }

    int sign3 = CheckLine(o, p1, p2);
    if (sign3 < 0)
    {
        return +1;
    }

    return (((sign1 != 0) && (sign2 != 0) && (sign3 != 0)) ? -1 : 0);
}
//}

さて、CombinePolygonの続きです。交点を見つけた後に、三角形の中に他の頂点が入っているか判定しますが、外積を使って内外判定をする関係上、三角形の接続の向きが時計回りになるようにしています。

//emlist[CombinePolygon関数の中盤１]{
if (GeomUtil.GetIntersectionPoint(M,
  new Vector3(maxX + 0.1f, M.y, M.z), outer,
  ref intersectionPoint, ref index0, ref index1))
{
    // 交点発見

    // 交差した線分の一番右の頂点を取得
    int pindex;
    Vector3[] triangle = new Vector3[3];
    if (outer[index0].x > outer[index1].x)
    {
        pindex = index0;
        // 選択した線分の頂点によって三角形が逆向きになってしまうので、時計回りになるように調整する
        triangle[0] = M;
        triangle[1] = outer[pindex];
        triangle[2] = intersectionPoint;
    }
    else
    {
        pindex = index1;
        triangle[0] = M;
        triangle[1] = intersectionPoint;
        triangle[2] = outer[pindex];
    }
//}

交点Iと線分の最もX座標が大きい頂点が同一だった場合、頂点Mから見て遮るものが無いということなので、三角形の中に他の頂点が含まれているかのチェックは行いません。
同一でなかった場合は、他の頂点が含まれているかのチェックを行いますが、三角形に含まれる頂点は、内側に凹んだ頂点なので、その条件を満たしつつ内包判定を行います。
三角形に複数の頂点が含まれていた場合、線分M,Iと線分M,該当頂点のなす角が最も小さい頂点を選択して、finalIndexに格納します。

//emlist[CombinePolygon関数の中盤２]{

Vector3 P = outer[pindex];

int finalIndex = pindex;

// 交点とPが同じだったら遮るものがないので三角形チェックしない
if((Vector3.Distance(intersectionPoint, P) > float.Epsilon))
{
    float angleMin = 361f;

    for(int i = 0; i < outer.Length; i++)
    {

        // 凸頂点/反射頂点チェック
        int prevIndex = (i == 0) ? outer.Length - 1 : i - 1; // 一つ前の頂点
        int nextIndex = (i + 1) % outer.Length;        // 次の頂点
        int nowIndex = i;

        if (nowIndex == pindex) continue;

        Vector3 outerP = outer[nowIndex];

        if (outerP.x < M.x) continue;

        // 分割時に複製した同一座標だったら無視
        if (Vector3.Distance(outerP, P) <= float.Epsilon) continue;

        Vector3 prevVertex = outer[prevIndex];
        Vector3 nextVertex = outer[nextIndex];
        Vector3 nowVertex = outer[nowIndex];

        // 反射頂点か？
        bool isReflex = !GeomUtil.IsAngleLessPI(nowVertex,
                                                prevVertex,
                                                nextVertex);

        // 三角形の中に「反射頂点」が含まれているか？
        if ((GeomUtil.IsInTriangle(outerP,
                                   triangle[0],
                                   triangle[1],
                                   triangle[2]) <= 0)&&(isReflex))
        {
            // 三角形の中に頂点が含まれてるので不可視

            // M,IとM,outerPの線分のなす角度を求める（一番角度が浅い頂点を選択する）
            float angle = Vector3.Angle(intersectionPoint - M, outerP - M);
            if (angle < angleMin)
            {
                angleMin = angle;
                finalIndex = nowIndex;
            }
        }
    }
}
//}

結合させる頂点（finalIndex）を探しだしたら、内外多角形の頂点配列をつなぎ合わせます。

 1. 新しい頂点配列のリストを作成します。
 2. リストに外側の多角形の結合させる頂点（finalIndex）までを追加します。
 3. 結合する内側の多角形の頂点配列を、頂点Mから順番に一周するようにリストに追加します。
 4. 再び内側の多角形から外側の多角形につなげる為、頂点Mと結合させる頂点（finalIndex）を複製して、リストに追加します。
 5. 残りの外側の多角形の頂点をリストに追加して完了です。

//emlist[CombinePolygonの後半]{
Vector3 FinalP = outer[finalIndex];

// 結合（新しい多角形を作成）
List<Vector3> newOuterVertices = new List<Vector3>();

// outerを分割するIndexまで追加
for (int i = 0; i <= finalIndex; i++)
{
    newOuterVertices.Add(outer[i]);
}

// innerをすべて追加
for (int i = xmaxIndex; i < inner.vertices.Length; i++)
{
    newOuterVertices.Add(inner.vertices[i]);
}
for (int i = 0; i < xmaxIndex; i++)
{
    newOuterVertices.Add(inner.vertices[i]);
}

// 分割するために頂点を2つ増やす
newOuterVertices.Add(M);
newOuterVertices.Add(FinalP);

// 残りのouterのindexを追加
for (int i = finalIndex + 1; i < outer.Length; i++)
{
    newOuterVertices.Add(outer[i]);
}

outer = newOuterVertices.ToArray();
//}

==== 三角形分割

内外多角形が一つの多角形になったら、いよいよ三角形分割です。
まず、頂点のインデックス配列の初期化や、耳リストの作成を行います。

//emlist[InitializeVertices関数]{
/// <summary>
/// 初期化
/// </summary>
void InitializeVertices(Vector3[] points)
{
    vertices.Clear();
    indices.Clear();
    earTipList.Clear();

    // インデックス配列の作成
    resultTriangulationOffset = resultVertices.Count;
    for (int i = 0; i < points.Length; i++)
    {
        Vector3 nowVertex = points[i];
        vertices.Add(nowVertex);

        indices.AddLast(i);

        resultVertices.Add(nowVertex);
    }

    // 凸三角形と耳の検索
    LinkedListNode<int> node = indices.First;
    while (node != null)
    {
        CheckVertex(node);
        node = node.Next;
    }
}
//}

頂点が耳かどうか判定するCheckVertex関数は次のようになっています。

//emlist[CheckVertex関数]{
void CheckVertex(LinkedListNode<int> node)
{
    // 凸頂点/反射頂点チェック
    int prevIndex = (node.Previous == null) ?
                      indices.Last.Value :
                      node.Previous.Value; // 一つ前の頂点
    int nextIndex = (node.Next == null) ?
                      indices.First.Value :
                      node.Next.Value;        // 次の頂点
    int nowIndex = node.Value;

    Vector3 prevVertex = vertices[prevIndex];
    Vector3 nextVertex = vertices[nextIndex];
    Vector3 nowVertex = vertices[nowIndex];

    bool isEar = false;

    // 内角が180度以内か？
    if (GeomUtil.IsAngleLessPI(nowVertex, prevVertex, nextVertex))
    {
        // 耳チェック
        // 180度以内、三角形の中に他の頂点が含まれない
        isEar = true;
        foreach(int i in indices)
        {
            if ((i == prevIndex) || (i == nowIndex) || (i == nextIndex))
                continue;

            Vector3 p = vertices[i];

            // 分割時に複製した同一座標だったら無視
            if (Vector3.Distance(p, prevVertex) <= float.Epsilon) continue;
            if (Vector3.Distance(p, nowVertex) <= float.Epsilon) continue;
            if (Vector3.Distance(p, nextVertex) <= float.Epsilon) continue;

            if(GeomUtil.IsInTriangle(p,
                                     prevVertex,
                                     nowVertex,
                                     nextVertex) <= 0)
            {
                isEar = false;
                break;
            }

        }
        if (isEar)
        {
            if (!earTipList.Contains(nowIndex))
            {
                // 耳追加
                earTipList.Add(nowIndex);
            }
        }
        else
        {
            // すでに耳のときに耳ではなくなった場合除外
            if (earTipList.Contains(nowIndex))
            {
                // 耳削除
                earTipList.Remove(nowIndex);
            }
        }

    }

}
//}

//embed[latex]{
\clearpage
//}

実際の三角形分割は次のEarClipping関数の中で行います。
前述したとおり、耳リストの先頭から頂点を取り出し、前後の頂点と結んだ三角形を出力。そして、頂点インデックス配列から耳の頂点を削除し、前後の頂点が耳になるか判定するという手順を繰り返します。

//emlist[EarClipping関数]{
void EarClipping()
{
    int triangleIndex = 0;

    while (earTipList.Count > 0)
    {
        int nowIndex = earTipList[0];   // 先頭取り出し

        LinkedListNode<int> indexNode = indices.Find(nowIndex);
        if (indexNode != null)
        {
            int prevIndex = (indexNode.Previous == null) ?
                              indices.Last.Value :
                              indexNode.Previous.Value; // 一つ前の頂点
            int nextIndex = (indexNode.Next == null) ?
                              indices.First.Value :
                              indexNode.Next.Value;        // 次の頂点

            Vector3 prevVertex = vertices[prevIndex];
            Vector3 nextVertex = vertices[nextIndex];
            Vector3 nowVertex = vertices[nowIndex];

            // 三角形作成
            triangles.Add(new Triangle(
              prevVertex,
              nowVertex,
              nextVertex, "(" + triangleIndex + ")"));

            resultTriangulation.Add(resultTriangulationOffset + prevIndex);
            resultTriangulation.Add(resultTriangulationOffset + nowIndex);
            resultTriangulation.Add(resultTriangulationOffset + nextIndex);

            triangleIndex++;

            if (indices.Count == 3)
            {
                // 最後の三角形なので終了
                break;
            }

            // 耳の頂点削除
            earTipList.RemoveAt(0);     // 先頭削除
            indices.Remove(nowIndex);

            // 前後の頂点のチェック
            int[] bothlist = { prevIndex, nextIndex };
            for (int i = 0; i < bothlist.Length; i++)
            {
                LinkedListNode<int> node = indices.Find(bothlist[i]);
                CheckVertex(node);
            }
        }
        else
        {
            Debug.LogError("index now found");
            break;
        }
    }

    // UV計算
    for (int i = 0; i < vertices.Count; i++)
    {
        Vector2 uv2 = CalcUV(vertices[i], uvRect);
        resultUVs.Add(uv2);
    }
}
//}

==== メッシュ生成

三角形分割した結果をMeshにします。
EarClipping関数の中で、必要な頂点配列とインデックス配列（resultVerticesとresultTriangulation）を用意して、Meshに流し込んでいます。

//emlist[MakeMesh関数]{
void MakeMesh()
{
    mesh = new Mesh();
    mesh.vertices = resultVertices.ToArray();
    mesh.SetIndices(resultTriangulation.ToArray(),
                    MeshTopology.Triangles, 0);
    mesh.RecalculateNormals();
    mesh.SetUVs(0, resultUVs);

    mesh.RecalculateBounds();

    MeshFilter filter = GetComponent<MeshFilter>();
    if(filter != null)
    {
        filter.mesh = mesh;
    }
}
//}

ついでに、UV座標もセットしています。UV座標は、多角形の矩形領域の中で割り当てるようにしてます。

//emlist[CalcUV]{
Vector2 CalcUV(Vector3 vertex, Rect uvRect)
{
    float u = (vertex.x - uvRect.x) / uvRect.width;
    float v = (vertex.y - uvRect.y) / uvRect.height;

    return new Vector2(u, v);
}
//}

== まとめ

耳刈り取り法による多角形の三角形分割について説明してきました。肝心の使いみちですが、マウスで図形を書いたらリアルタイムにメッシュになる、フォントのアウトラインデータをメッシュにする、等が思いつきますが、そんなに高速なアルゴリズムではないので頂点数が増えてくると速度面で問題が出てくるでしょう（CPUで順番に計算してるし）それでも、複雑な多角形がシンプルなアルゴリズムで三角形に分割できるのは、興味深いと思います。

== 参照

 * Triangulation by Ear Clipping @<br>{}
@<href>{https://www.geometrictools.com/Documentation/TriangulationByEarClipping.pdf}

 * 多角形の三角形分割 @<br>{}
@<href>{https://ja.wikipedia.org/wiki/多角形の三角形分割}
