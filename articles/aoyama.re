= Poisson Disk Sampling


== はじめに
本章では、Poisson Disk Sampling（ポアソンディスクサンプリング：以下、「PDS」を使用）の概要とCPUでの実装アルゴリズムの説明をおこないます。

CPUにおける実装にはFast Poisson Disk Sampling in Arbitary Dimensions（任意次元における高速ポアソンディスクサンプリング：以下、「FPDS」を使用）を採用しています。
このアルゴリズムは、ブリティッシュ・コロンビア大学のロバート・ブリッドソン氏によって、2007年のSIGGRAPHに提出された論文にて提案されました。

== 概要
そもそもPDSとは何なのかご存じない方もいらっしゃると思いますので、本節ではPDSについて説明します。

まず、適当な空間に多数の点をプロットしていきます。次に、0より大きい距離@<m>{d}を考えます。
このとき、@<img>{aoyama/random_point}に示すように全ての点がランダムな位置にありながら、全ての点同士が最低でも@<m>{d}以上離れているような分布を、Poisson-disk分布といいます。
つまり、@<img>{aoyama/random_point}からランダムに2点を選んだとして、どの組み合わせにおいても、必ずその間の距離が@<m>{d}より小さくなることはありません。
このようなPoisson-disk分布をサンプリングすること、言い換えればPoisson-disk分布を計算によって生成することを、PDSといいます。

このPDSによって、一様性をもつランダムな点群を得ることができます。
そのため、アンチエイリアスを始めとするフィルタリング処理におけるサンプリングや、テクスチャ合成処理での合成ピクセルを決定するためのサンプリングなどに利用されています。

//image[aoyama/random_point][ランダムな点をプロット][scale=0.5]

== Fast Poisson Disk Sampling in Arbitary Dimensions （CPU実装）
FPDSの最大の特徴は、次元に依存しないという点です。
これまで提案されてきた手法のほとんどは二次元上での計算を前提としているものであり、効率的に三次元上でのサンプリングをおこなうことができませんでした。
そこで、任意の次元において、高速に計算を行うために提案されたのがFPDSです。

FPDSはO(N)で計算することができ、またどの次元においてでも、同一のアルゴリズムに基づきて実装することができます。
本節では、FPDSの処理を順を追って説明します。

=== FPDSの導入
FPDSの計算において、3つのパラメーターを用います。

 * サンプリング空間の範囲 ： @<m>{Rn}
 * 点同士の最小距離 ： @<m>{r}
 * サンプリング限界数 ： @<m>{k}

これらのパラメーターは、利用者によって自由に入力することができます。
以降の説明においても上記のパラメーターを利用します。

=== サンプリング空間をGridに分割する
点同士の距離の計算を高速化するために、サンプリング空間をGridに分割します。
ここで、サンプリング空間の次元数を@<m>{n}として、分割されてできた各分割空間（以下、「セル」とする）のサイズは@<m>{\frac{r\}{\sqrt{n\}\}}とします。
@<img>{aoyama/euclid}に示すように@<m>{\sqrt{n\}}は、@<m>{n}次元における各軸の長さが1のベクトルの絶対値を表しています。
//image[aoyama/euclid][n=3のとき][scale=0.75]

そして、サンプリングした点はその座標が含まれているセルに所属させます。
各セルのサイズは@<m>{\frac{r\}{\sqrt{n\}\}}となっているため、隣り合っているセル同士の中心距離も@<m>{\frac{r\}{\sqrt{n\}\}}となります。
つまり@<m>{\frac{r\}{\sqrt{n\}\}}によって空間を分割することで、@<img>{aoyama/grid}から分かるように各セルには最大でも一点だけが所属することになります。
さらに、あるセルの周辺を近傍探索するときは@<m>{\pm{n\}}だけ周辺セルを探索することで、最小距離@<m>{r}以内の距離にある全てのセルを探索できることになります。

//image[aoyama/grid][n=2における平面分割][scale=0.75]

また、このGridはn次元の配列によって表現することが可能なため、とても実装が簡単です。
サンプリングした点を、その座標に対応するセルに所属させることで、近くに存在する他点の探索を容易におこなえます。

//emlist[ ][cs]{
// 3次元グリッドを表現する3次元配列を取得する
Vector3?[, ,] GetGrid(Vector3 bottomLeftBack, Vector3 topRightForward
    , float min, int iteration)
{
    // サンプリング空間
    var dimension = (topRightForward - bottomLeftBack);
    // 最小距離に√3の逆数をかける（割り算を避けたい）
    var cell = min * InvertRootTwo;

    return new Vector3?[
        Mathf.CeilToInt(dimension.x / cell) + 1,
        Mathf.CeilToInt(dimension.y / cell) + 1,
        Mathf.CeilToInt(dimension.z / cell) + 1
    ];
}

// 座標に対応するセルのIndexを取得する
Vector3Int GetGridIndex(Vector3 point, Settings set)
{
    // 基準点からの距離をセルサイズで割ってIndexを算出
    return new Vector3Int(
        Mathf.FloorToInt((point.x - set.BottomLeftBack.x) / set.CellSize),
        Mathf.FloorToInt((point.y - set.BottomLeftBack.y) / set.CellSize),
        Mathf.FloorToInt((point.z - set.BottomLeftBack.z) / set.CellSize)
    );
}
//}

=== 初期のサンプル点を計算する
計算の起点となる、最初のサンプル点を計算します。
この時点では、どの座標にサンプリングをおこなっても距離@<m>{r}より近い他点は存在しないので、完全にランダムな座標を一点決めます。

この計算したサンプル点の座標を元に、対応するセルに所属させ、さらにアクティブリストとサンプリングリストに追加します。
このアクティブリストは、サンプリングをおこなう際に起点となる点を保存しておくリストです。
このアクティブリストに保存されている点を基準に、順次サンプリングをおこなっていきます。

//emlist[ ][cs]{
// ランダムな座標を一点求める
void GetFirstPoint(Settings set, Bags bags)
{
    var first = new Vector3(
        Random.Range(set.BottomLeftBack.x, set.TopRightForward.x),
        Random.Range(set.BottomLeftBack.y, set.TopRightForward.y),
        Random.Range(set.BottomLeftBack.z, set.TopRightForward.z)
    );
    var index = GetGridIndex(first, set);

    bags.Grid[index.x, index.y, index.z] = first;
    // サンプリングリスト、最終的にはこのListを結果として返す
    bags.SamplePoints.Add(first);
    // アクティブリスト、このListを基準にサンプリングをまわす
    bags.ActivePoints.Add(first);
}
//}

=== アクティブリストから基準となる点を選ぶ
アクティブリストからランダムにIndex@<m>{i}を選び、@<m>{i}に保存されている座標を@<m>{x_i}とします。
（当然ですが一番最初のときは@<hd>{初期のサンプル点を計算する}にて生成した点だけなので、@<m>{i}は0になります。）

この@<m>{x_i}を中心に、付近の座標に対して他点をサンプリングしていくことになります。
これを繰り返していくことで、空間全体をサンプリングすることができます。

//image[aoyama/first_darts][初期のサンプリング点を選択][scale=0.5]

//emlist[ ][cs]{
// アクティブリストからランダムに点を選ぶ
var index = Random.Range(0, bags.ActivePoints.Count);
var point = bags.ActivePoints[index];
//}

=== サンプリングする
@<m>{x_i} を中心として半径@<m>{r}以上、@<m>{2r}以下のn次元球面（2次元なら円、3次元なら球になります）内で、ランダムな座標@<m>{x^{\prime\}_i}を計算します。
次に、計算した@<m>{x^{\prime\}_i}の周辺に距離が@<m>{r}より近い他点が存在するかを調べます。

ここで、全ての他点について距離計算をおこなうのは、他点の数が多いほど負荷の高い処理となります。
そこで、この問題を解決するために@<hd>{サンプリング空間をGridに分割する}にて生成したGridを用い、@<m>{x^{\prime\}_i} が所属するセルの周辺セルだけを調べることで、計算量を削減します。
周辺セルに他点が存在するときは@<m>{x^{\prime\}_i}を破棄し、他点が存在しなかったときは@<m>{x^{\prime\}_i}を対応するセルに所属させ、アクティブリストとサンプリングリストに追加します。

//image[aoyama/first_sampling][初期のサンプリング点を基準に他点をサンプリング][scale=0.5]

//emlist[ ][cs]{
// point座標を元に、次のサンプリング点を求める
private static bool GetNextPoint(Vector3 point, Settings set, Bags bags)
{
    // point座標を中心にr ~ 2rの範囲にあるランダムな点を一点求める
    var p = point +
        GetRandPosInSphere(set.MinimumDistance, 2f * set.MinimumDistance);

    // サンプリング空間の範囲外だった場合はサンプリング失敗扱いにする
    if(set.Dimension.Contains(p) == false) { return false; }

    var minimum = set.MinimumDistance * set.MinimumDistance;
    var index = GetGridIndex(p, set);
    var drop = false;

    // 探索するGridの範囲を計算
    var around = 3;
    var fieldMin = new Vector3Int(
        Mathf.Max(0, index.x - around), Mathf.Max(0, index.y - around),
        Mathf.Max(0, index.z - around)
    );
    var fieldMax = new Vector3Int(
        Mathf.Min(set.GridWidth, index.x + around),
        Mathf.Min(set.GridHeight, index.y + around),
        Mathf.Min(set.GridDepth, index.z + around)
    );

    // 周辺のGridに他点が存在しているかを確認
    for(var i = fieldMin.x; i <= fieldMax.x && drop == false; i++)
    {
        for(var j = fieldMin.y; j <= fieldMax.y && drop == false; j++)
        {
            for(var k = fieldMin.z; k <= fieldMax.z && drop == false; k++)
            {
                var q = bags.Grid[i, j, k];
                if(q.HasValue && (q.Value - p).sqrMagnitude <= minimum)
                {
                    drop = true;
                }
            }
        }
    }

    if(drop == true) { return false; }

    // 周辺に他点が存在していないので採用
    bags.SamplePoints.Add(p);
    bags.ActivePoints.Add(p);
    bags.Grid[index.x, index.y, index.z] = p;
    return true;
}
//}

=== サンプリングを繰り返す
@<m>{x_i}を中心に、@<hd>{サンプリングする}にて一点だけ他点をサンプリングしましたが、これを@<m>{k}回繰り返します。
もし@<m>{x_i}について@<m>{k}回繰り返して、一点もサンプリングすることができなかった場合は、@<m>{x_i}をアクティブリストから削除します。

//image[aoyama/looped_sampling][@<m>{k = 7}のとき][scale=0.5]

そして@<m>{k}の繰り返しが終わったら、また@<hd>{アクティブリストから基準となる点を選ぶ}に戻ります。
これをアクティブリストが0になるまで繰り返すことで、空間全体をサンプリングすることができます。

//emlist[ ][cs]{
// サンプリングを繰り返す
public static List<Vector3> Sampling(Vector3 bottomLeft, Vector3 topRight,
    float minimumDistance, int iterationPerPoint)
{
    var settings = GetSettings(
        bottomLeft,
        topRight,
        minimumDistance,
        iterationPerPoint <= 0 ?
            DefaultIterationPerPoint : iterationPerPoint
    );
    var bags = new Bags()
    {
        Grid = new Vector3?[
            settings.GridWidth + 1,
            settings.GridHeight + 1,
            settings.GridDepth + 1
        ],
        SamplePoints = new List<Vector3>(),
        ActivePoints = new List<Vector3>()
    };
    GetFirstPoint(settings, bags);

    do
    {
        var index = Random.Range(0, bags.ActivePoints.Count);
        var point = bags.ActivePoints[index];

        var found = false;
        for(var k = 0; k < settings.IterationPerPoint; k++)
        {
            found = found | GetNextPoint(point, settings, bags);
        }

        if(found == false) { bags.ActivePoints.RemoveAt(index); }
    }
    while(bags.ActivePoints.Count > 0);

    return bags.SamplePoints;
}
//}

つまり、簡単に全体の流れを説明すると

 * 空間を決めてグリッドに分割する
 * 適当に初期点を決める
 * 初期点の周辺にサンプリングする
 * サンプリングした点についても、同様に周辺をサンプリングする
 * すべての点について、周辺のサンプリングができたら終了

ということになります。
本アルゴリズムでは並列化の提案などはされていないため、サンプリング空間@<m>{Rn}が広かったり、最小距離@<m>{r}が小さかったりするとそれなりの計算時間が必要ですが、任意次元において簡単にサンプリングすることができるというのは、非常に魅力的な利点です。

== まとめ
ここまでの処理によって@<img>{aoyama/result}のようにサンプリング結果を得ることができます。
この画像は、サンプリングされた座標にGeometryShaderで円を生成し、配置した物です。
円同士が重なることがなく、範囲いっぱいに埋め尽くされているのが分かります。
//image[aoyama/result][サンプリング結果の可視化][scale=0.5]

冒頭でも少し述べましたが、ポアソンディスクサンプリングはアンチエイリアスや画像合成を始めとして、Blurなどのイメージエフェクトからオブジェクトの等間隔配置まで、幅広い場所で活用されています。
これ単体で明確な見た目の成果を得られたりする訳ではありませんが、私たちが普段目にしているハイクオリティなビジュアルの裏では頻繁に利用されています。
ビジュアルプログラミングをおこなっていく上で、知っておいて損のないアルゴリズムのひとつといえるでしょう。

== 参考
 * Fast Poisson Disk Sampling in Arbitrary Dimensions - @<href>{https://www.cct.lsu.edu/~fharhad/ganbatte/siggraph2007/CD2/content/sketches/0250.pdf}
