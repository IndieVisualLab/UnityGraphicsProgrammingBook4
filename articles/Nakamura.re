
= GPU-Based Space Colonization Algorithm

本章では、点群に沿ってブランチングする形状を生成するアルゴリズム、Space Colonization AlgorithmのGPU実装とその応用例を紹介します。

本章のサンプルは@<br>{}
@<href>{https://github.com/IndieVisualLab/UnityGraphicsProgramming4}@<br>{}
の「SpaceColonization」です。

//image[nakamura/SkinnedAnimationScene][SkinnedAnimation.scene]

== はじめに

Space Colonization Algorithmは、
Adamら@<fn>{Adam}による樹木のモデリング手法として開発されました。

//footnote[Adam][http://algorithmicbotany.org/papers/colonization.egwnp2007.html]

与えられた点群からブランチングする形状を生成する手法で、

 * 枝同士が密着しすぎず、程よくばらけさせながらブランチングさせられる
 * 初期の点群配置によって枝の配置が決まるので、形状をコントロールしやすい
 * シンプルなパラメータで枝の粗密具合をコントロールできる

という特徴を持ちます。

本章ではこのアルゴリズムのGPU実装と、スキニングアニメーションと組み合わせた応用例について紹介します。

== アルゴリズム

まずはSpace Colonization Algorithmを解説します。
アルゴリズムの大まかなステップは以下のように分けられます。

 1. Setup - 初期化
 2. Search - 影響するAttractionの検索
 3. Attract - ブランチの引き寄せ
 4. Connect - 新規Nodeの生成と既存Nodeとの接続
 5. Remove - Attractionの削除
 6. Grow - Nodeの成長

=== Setup - 初期化

初期化フェーズでは、点群をAttraction（ブランチのシードとなる点）として用意します。
そのAttractionが散りばめられた中に、Node（ブランチの分岐点）を1つ以上配置します。
この最初に配置されたNodeがブランチの開始点となります。

以下の図中では、Attractionを丸い点、Nodeを四角い点で表しています。

//image[nakamura/Step1][Setup - AttractionとNodeの初期化　丸い点がAttraction、四角い点がNodeを表している]

=== Search - 影響するAttractionの検索

それぞれのAttractionについて、影響範囲（influence distance）内で最も近傍にあるNodeを検索します。

//image[nakamura/Step2-gpu][Search - それぞれのAttractionから影響範囲内で最も近傍のNodeを検索する]

=== Attract - ブランチの引き寄せ

それぞれのNodeについて、影響範囲内にあるAttractionを元にブランチを伸ばす方向を決め、
成長の長さ（growth distance）分だけ伸ばした先の点を、
新たにNodeを生成する点の候補点（Candidate）とします。

//image[nakamura/Step3-gpu][Attract - それぞれのNodeからブランチを伸ばして新たにNodeを生成する候補点を決める]

=== Connect - 新たなNodeと既存のNodeとの接続

Candidateの位置に新たなNodeを生成し、
元のNodeとEdgeで繋いでブランチを拡張します。

//image[nakamura/Step4][Connect - 新たなNodeと既存Nodeとを接続してブランチを拡張する]

=== Remove - 削除範囲内にあるAttractionの削除

Nodeから削除範囲（kill distance）内にあるAttractionを削除します。

//image[nakamura/Step5_1][Remove - Nodeから削除範囲内にあるAttractionを検索する]
//image[nakamura/Step5_2][Remove - 削除範囲内に見つかったAttractionを削除する]


=== Grow - Nodeの成長

Nodeを成長させ、Step.2に戻ります。

アルゴリズム全体の大まかな流れは以下の図のようになります。

//image[nakamura/Flow][アルゴリズムの大まかな流れ]

== 実装

それではアルゴリズムの具体的な実装について解説していきます。

=== リソースの用意

Space Colonization Algorithmでは増減する要素として

 * Attraction : ブランチのシードとなる点群
 * Node : ブランチの分岐点（ノード）
 * Candidate : 新たにノードを生成する候補となる点
 * Edge : ブランチのノード間をつなぐエッジ

が必要となりますが、これらをGPGPU上で表現するために、
いくつかの要素にAppend/ConsumeStructuredBufferを利用します。

Append/ConsumeStructuredBufferについては
Unity Graphics Programming vol.3「GPU-Based Cellular Growth Simulation」で解説しています。

==== Attraction（ブランチのシードとなる点）

Attractionの構造は以下のように定義します。

//emlist[Attraction.cs]{
public struct Attraction {
    public Vector3 position; // 位置
    public int nearest; // 最近傍Nodeのindex
    public uint found; // 近傍Nodeが見つかったかどうか
    public uint active; // 有効なAttractionかどうか(1なら有効、0なら削除済み)
}
//}

activeフラグによって削除済みのAttractionかどうかを判別することで、
Attractionの増減を表現します。

Space ColonizationではAttractionの点群を初期化フェーズで用意する必要があります。
サンプルのSpaceColonization.csでは球形の内部に点群をランダムに散りばめ、
Attractionの位置として利用しています。

//emlist[SpaceColonization.cs]{
    // 球形の内部にランダムに点を散りばめてAttractionを生成する
    var attractions = GenerateSphereAttractions();
    count = attractions.Length;

    // Attractionバッファの初期化
    attractionBuffer = new ComputeBuffer(
        count, 
        Marshal.SizeOf(typeof(Attraction)), 
        ComputeBufferType.Default
    );
    attractionBuffer.SetData(attractions);
//}

==== Node（ブランチの分岐点）

Nodeの構造は以下のように定義します。

//emlist[Node.cs]{
public struct Node {
    public Vector3 position; // 位置
    public float t; // 成長率(0.0 ~ 1.0)
    public float offset; // Rootからの距離(Nodeの深さ)
    public float mass; // 質量
    public int from; // 分岐元Nodeのindex
    public uint active; // 有効なNodeかどうか(1なら有効)
}
//}

Nodeのリソースは

 * Nodeの実データを表すバッファ
 * オブジェクトプールとして利用する、activeでないNodeのindexを管理するバッファ

の2つのバッファで管理します。

//emlist[SpaceColonization.cs]{
  // Nodeの実データ
  nodeBuffer = new ComputeBuffer(
    count, 
    Marshal.SizeOf(typeof(Node)), 
    ComputeBufferType.Default
  );

  // オブジェクトプール
  nodePoolBuffer = new ComputeBuffer(
    count, 
    Marshal.SizeOf(typeof(int)), 
    ComputeBufferType.Append
  );
  nodePoolBuffer.SetCounterValue(0);
//}

==== Candidate（新たなNodeの候補点）

Candidateの構造は以下のように定義します。

//emlist[Candidate.cs]{
public struct Candidate 
{
    public Vector3 position; // 位置
    public int node; // 候補点の元Nodeのindex
}
//}

CandidateはAppend/ConsumeStructuredBufferで表現します。

//emlist[SpaceColonization.cs]{
    candidateBuffer = new ComputeBuffer(
        count, 
        Marshal.SizeOf(typeof(Candidate)), 
        ComputeBufferType.Append
    );
    candidateBuffer.SetCounterValue(0);
//}

==== Edge（Node間をつなぐEdge）

Edgeの構造は以下のように定義します。

//emlist[Edge.cs]{
public struct Edge {
    public int a, b; // Edgeが繋ぐ2つのNodeのindex
}
//}

EdgeはCandidateと同様にAppend/ConsumeStructuredBufferで表現します。

//emlist[SpaceColonization.cs]{
    edgeBuffer = new ComputeBuffer(
        count * 2, 
        Marshal.SizeOf(typeof(Edge)), 
        ComputeBufferType.Append
    );
    edgeBuffer.SetCounterValue(0);
//}

=== ComputeShaderでのアルゴリズムの実装

これで必要なリソースがそろったので、
アルゴリズムの各ステップをComputeShaderによるGPGPUで実装していきます。

==== Setup 

初期化フェーズでは、

 * Nodeのオブジェクトプールの初期化
 * 初期Nodeをシードとして追加

を行います。

用意したAttractionからいくつかをピックアップし、
その位置に初期Nodeを生成します。

//emlist[SpaceColonization.cs]{
    var seeds = Enumerable.Range(0, seedCount).Select((_) => { 
        return attractions[Random.Range(0, count)].position; 
    }).ToArray();
    Setup(seeds);
//}

//emlist[SpaceColonization.cs]{
protected void Setup(Vector3[] seeds)
{
    var kernel = compute.FindKernel("Setup");
    compute.SetBuffer(kernel, "_NodesPoolAppend", nodePoolBuffer);
    compute.SetBuffer(kernel, "_Nodes", nodeBuffer);
    GPUHelper.Dispatch1D(compute, kernel, count);

    ...
}
//}

Setupカーネルではオブジェクトプールの初期化を行います。
Nodeのオブジェクトプールにindexを格納し、該当するNodeのactiveフラグをオフにします。

//emlist[SpaceColonization.compute]{
void Setup (uint3 id : SV_DispatchThreadID)
{
  uint idx = id.x;
  uint count, stride;
  _Nodes.GetDimensions(count, stride);
  if (idx >= count)
    return;

  _NodesPoolAppend.Append(idx);

  Node n = _Nodes[idx];
  n.active = false;
  _Nodes[idx] = n;
}
//}

これですべてのNodeのactiveフラグがオフの状態となり、
Nodeのindexを持ったオブジェクトプールが生成されます。

オブジェクトプールの初期化が済んだので、
次はシードとなる初期ノードを生成します。

先ほど用意したシード位置（Vector3[]）を入力として、
Seedカーネルを実行することで初期ノードを生成します。

//emlist[SpaceColonization.cs]{
... 

// seedBufferはスコープを抜けると自動的にDisposeされる
using(
    ComputeBuffer seedBuffer = new ComputeBuffer(
        seeds.Length, 
        Marshal.SizeOf(typeof(Vector3))
    )
)
{
    seedBuffer.SetData(seeds);
    kernel = compute.FindKernel("Seed");
    compute.SetFloat("_MassMin", massMin);
    compute.SetFloat("_MassMax", massMax);
    compute.SetBuffer(kernel, "_Seeds", seedBuffer);
    compute.SetBuffer(kernel, "_NodesPoolConsume", nodePoolBuffer);
    compute.SetBuffer(kernel, "_Nodes", nodeBuffer);
    GPUHelper.Dispatch1D(compute, kernel, seedBuffer.count);
}

// NodeとEdgeの数の初期化
nodesCount = nodePoolBuffer.count;
edgesCount = 0;

...
//}

Seedカーネルは、
Seedsバッファから位置を取り出し、その位置にNodeを生成します。

//emlist[SpaceColonization.compute]{
void Seed (uint3 id : SV_DispatchThreadID)
{
  uint idx = id.x;

  uint count, stride;
  _Seeds.GetDimensions(count, stride);
  if (idx >= count)
    return;

  Node n;

  // 新たなNodeを生成(後述)
  uint i = CreateNode(n);

  // Seedの位置をNodeのpositionに設定
  n.position = _Seeds[idx];
  n.t = 1;
  n.offset = 0;
  n.from = -1;
  n.mass = lerp(_MassMin, _MassMax, nrand(id.xy));
  _Nodes[i] = n;
}
//}

CreateNode関数で新たなNodeを生成します。
オブジェクトプールであるConsumeStructuredBufferからindexを取り出し、
初期化したNodeを返します。

//emlist[SpaceColonization.compute]{
uint CreateNode(out Node node)
{
  uint i = _NodesPoolConsume.Consume();
  node.position = float3(0, 0, 0);
  node.t = 0;
  node.offset = 0;
  node.from = -1;
  node.mass = 0;
  node.active = true;
  return i;
}
//}

これで初期化フェーズは終わりです。

@<img>{nakamura/Flow}で示しているループするアルゴリズムの各ステップはStep関数内で実行しています。

//emlist[SpaceColonization.cs]{

protected void Step(float dt)
{
    // オブジェクトプールが空のときは実行しないようにする
    if (nodesCount > 0)
    {
        Search();   // Step.2
        Attract();  // Step.3
        Connect();  // Step.4
        Remove();   // Step.5

        // Append/ConsumeStructuredBufferが持つデータ数を取得
        CopyNodesCount();
        CopyEdgesCount();
    }
    Grow(dt);       // Step.6
}

//}

==== Search

各Attractionから、影響範囲（influence distance)内で最も近傍にあるNodeを検索します。

//emlist[SpaceColonization.cs]{
protected void Search()
{
    var kernel = compute.FindKernel("Search");
    compute.SetBuffer(kernel, "_Attractions", attractionBuffer);
    compute.SetBuffer(kernel, "_Nodes", nodeBuffer);
    compute.SetFloat("_InfluenceDistance", unitDistance * influenceDistance);
    GPUHelper.Dispatch1D(compute, kernel, count);
}
//}

GPUカーネルの実装は以下の通りです。

//emlist[SpaceColonization.compute]{
void Search (uint3 id : SV_DispatchThreadID)
{
  uint idx = id.x;
  uint count, stride;
  _Attractions.GetDimensions(count, stride);
  if (idx >= count)
    return;

  Attraction attr = _Attractions[idx];

  attr.found = false;
  if (attr.active)
  {
    _Nodes.GetDimensions(count, stride);

    // influence distanceよりも近傍のNodeを探索する
    float min_dist = _InfluenceDistance;

    // 最も近傍のNodeのindex
    uint nearest = -1;

    // すべてのNodeについてループを実行
    for (uint i = 0; i < count; i++)
    {
      Node n = _Nodes[i];

      if (n.active)
      {
        float3 dir = attr.position - n.position;
        float d = length(dir);
        if (d < min_dist)
        {
          // 最も近傍のNodeを更新
          min_dist = d;
          nearest = i;

          // 近傍Nodeのindexを設定する
          attr.found = true;
          attr.nearest = nearest;
        }
      }
    }

    _Attractions[idx] = attr;
  }
}
//}

==== Attract

それぞれのNodeについて、影響範囲内にあるAttractionを元にブランチを伸ばす方向を決め、
成長の長さ（growth distance）分だけ伸ばした先の点を、
新たにNodeを生成する点の候補点（Candidate）とします。

//emlist[SpaceColonization.cs]{
protected void Attract()
{
    var kernel = compute.FindKernel("Attract");
    compute.SetBuffer(kernel, "_Attractions", attractionBuffer);
    compute.SetBuffer(kernel, "_Nodes", nodeBuffer);

    candidateBuffer.SetCounterValue(0); // 候補点を格納するバッファの初期化
    compute.SetBuffer(kernel, "_CandidatesAppend", candidateBuffer);

    compute.SetFloat("_GrowthDistance", unitDistance * growthDistance);

    GPUHelper.Dispatch1D(compute, kernel, count);
}
//}

GPUカーネルの実装は以下の通りです。
候補点の位置の計算方法はAttractカーネルのコード内容とコメントを参照してください。

//emlist[SpaceColonization.compute]{
void Attract (uint3 id : SV_DispatchThreadID)
{
  uint idx = id.x;
  uint count, stride;
  _Nodes.GetDimensions(count, stride);
  if (idx >= count)
    return;

  Node n = _Nodes[idx];

  // Nodeが有効かつ、
  // 成長率(t)が閾値(1.0)以上あれば新たなNodeを生成する
  if (n.active && n.t >= 1.0)
  {
    // ブランチを伸ばす先の累算用変数
    float3 dir = (0.0).xxx;
    uint counter = 0;

    // 全Attractionについてループを実行する
    _Attractions.GetDimensions(count, stride);
    for (uint i = 0; i < count; i++)
    {
      Attraction attr = _Attractions[i];
      // 該当Nodeが最近傍であるAttractionを検索する
      if (attr.active && attr.found && attr.nearest == idx)
      {
        // NodeからAttractionへ向かうベクトルを正規化して累算用変数に加算する
        float3 dir2 = (attr.position - n.position);
        dir += normalize(dir2);
        counter++;
      }
    }

    if (counter > 0)
    {
      Candidate c;

      // NodeからAttractionへ向かう単位ベクトルの平均を取り、
      // それをgrowth distance分、Nodeから伸ばした先を候補点の位置とする
      dir = dir / counter;
      c.position = n.position + (dir * _GrowthDistance);

      // 候補点へ伸びる元Nodeのindexを設定する
      c.node = idx;

      // 候補点バッファに加える
      _CandidatesAppend.Append(c);
    }
  }
}
//}

==== Connect

Attractカーネルで生成された候補点バッファを元に新たなNodeを生成し、
Node同士をEdgeで繋げることでブランチを拡張します。

Connect関数では、
Nodeオブジェクトプール（nodePoolBuffer）が空の状態でデータの取り出し（Consume）が実行されないよう、
オブジェクトプールの残り数（nodesCount）と候補点バッファのサイズを比較してカーネルの実行回数を決めています。

//emlist[SpaceColonization.cs]{
protected void Connect()
{
    var kernel = compute.FindKernel("Connect");
    compute.SetFloat("_MassMin", massMin);
    compute.SetFloat("_MassMax", massMax);
    compute.SetBuffer(kernel, "_Nodes", nodeBuffer);
    compute.SetBuffer(kernel, "_NodesPoolConsume", nodePoolBuffer);
    compute.SetBuffer(kernel, "_EdgesAppend", edgeBuffer);
    compute.SetBuffer(kernel, "_CandidatesConsume", candidateBuffer);

    // CopyNodeCountで取得したNodeオブジェクトプールの持つデータ数(nodeCount)を
    // 越えないように制限をかける
    var connectCount = Mathf.Min(nodesCount, CopyCount(candidateBuffer));
    if (connectCount > 0)
    {
        compute.SetInt("_ConnectCount", connectCount);
        GPUHelper.Dispatch1D(compute, kernel, connectCount);
    }
}
//}

以下がGPUカーネルの実装になります。

//emlist[SpaceColonization.compute]{
void Connect (uint3 id : SV_DispatchThreadID)
{
  uint idx = id.x;
  if (idx >= _ConnectCount)
    return;

  // 候補点バッファから候補点を取り出し
  Candidate c = _CandidatesConsume.Consume();

  Node n1 = _Nodes[c.node];
  Node n2;

  // 候補点の位置にNodeを生成
  uint idx2 = CreateNode(n2);
  n2.position = c.position;
  n2.offset = n1.offset + 1.0; // Rootからの距離を設定 (元Node + 1.0)
  n2.from = c.node; // 元Nodeのindexを設定
  n2.mass = lerp(_MassMin, _MassMax, nrand(float2(c.node, idx2)));

  // Nodeバッファを更新
  _Nodes[c.node] = n1;
  _Nodes[idx2] = n2;

  // 2つのNodeをEdgeで繋ぐ(後述)
  CreateEdge(c.node, idx2);
}
//}

CreateEdge関数は渡された2つのNodeのindexを元にEdgeを生成し、
Edgeバッファに追加します。

//emlist[SpaceColonization.compute]{
void CreateEdge(int a, int b)
{
  Edge e;
  e.a = a;
  e.b = b;
  _EdgesAppend.Append(e);
}
//}

==== Remove

Nodeからkill distance内にあるAttractionを削除します。

//emlist[SpaceColonization.cs]{
protected void Remove()
{
    var kernel = compute.FindKernel("Remove");
    compute.SetBuffer(kernel, "_Attractions", attractionBuffer);
    compute.SetBuffer(kernel, "_Nodes", nodeBuffer);
    compute.SetFloat("_KillDistance", unitDistance * killDistance);
    GPUHelper.Dispatch1D(compute, kernel, count);
}
//}

GPUカーネルの実装は以下の通りです。

//emlist[SpaceColonization.compute]{
void Remove(uint3 id : SV_DispatchThreadID)
{
  uint idx = id.x;
  uint count, stride;
  _Attractions.GetDimensions(count, stride);
  if (idx >= count)
    return;

  Attraction attr = _Attractions[idx];
  // 削除済みのAttractionの場合は実行しない
  if (!attr.active)
    return;

  // 全Nodeについてループを実行する
  _Nodes.GetDimensions(count, stride);
  for (uint i = 0; i < count; i++)
  {
    Node n = _Nodes[i];
    if (n.active)
    {
      // 削除範囲内のNodeがあれば、Attractionのactiveフラグをオフにして削除
      float d = distance(attr.position, n.position);
      if (d < _KillDistance)
      {
        attr.active = false;
        _Attractions[idx] = attr;
        return;
      }
    }
  }
}
//}

==== Grow

Nodeを成長させます。

Attractカーネルで候補点を生成する際、
Nodeの成長率（t）が閾値以上かどうかを条件に用いていますが（閾値以下の場合は候補点を生成しない）、
成長率パラメータはこのGrowカーネルで増分させています。

//emlist[SpaceColonization.cs]{
protected void Grow(float dt)
{
    var kernel = compute.FindKernel("Grow");
    compute.SetBuffer(kernel, "_Nodes", nodeBuffer);

    var delta = dt * growthSpeed;
    compute.SetFloat("_DT", delta);

    GPUHelper.Dispatch1D(compute, kernel, count);
}
//}

//emlist[SpaceColonization.compute]{
void Grow (uint3 id : SV_DispatchThreadID)
{
  uint idx = id.x;
  uint count, stride;
  _Nodes.GetDimensions(count, stride);
  if (idx >= count)
    return;

  Node n = _Nodes[idx];

  if (n.active)
  {
    // Nodeごとにランダムに設定されたmassパラメータで成長速度をバラバラにする
    n.t = saturate(n.t + _DT * n.mass);
    _Nodes[idx] = n;
  }
}
//}

=== レンダリング

以上までの実装でブランチングする形状が得られたので、
その形状をどうレンダリングするかについて解説します。

==== Line Topologyによるレンダリング

まずはシンプルにLine Meshを用いてレンダリングします。

1本のEdgeを表すLineを描画するために、シンプルなLine TopologyのMeshを生成します。

//emlist[SpaceColonization.cs]{
protected Mesh BuildSegment()
{
    var mesh = new Mesh();
    mesh.hideFlags = HideFlags.DontSave;
    mesh.vertices = new Vector3[2] { Vector3.zero, Vector3.up };
    mesh.uv = new Vector2[2] { new Vector2(0f, 0f), new Vector2(0f, 1f) };
    mesh.SetIndices(new int[2] { 0, 1 }, MeshTopology.Lines, 0);
    return mesh;
}
//}

//image[nakamura/Segment][シンプルな2頂点だけを持つLine TopologyのMesh]

2つの頂点だけを持つSegment（線分）を、GPU instancingを用いてEdgeの数だけレンダリングすることで生成したブランチを表示します。

//emlist[SpaceColonization.cs]{

// GPU instancingに必要な、レンダリングするMeshの数を決めるバッファを生成する
protected void SetupDrawArgumentsBuffers(int count)
{
    if (drawArgs[1] == (uint)count) return;

    drawArgs[0] = segment.GetIndexCount(0);
    drawArgs[1] = (uint)count;

    if (drawBuffer != null) drawBuffer.Dispose();
    drawBuffer = new ComputeBuffer(
        1, 
        sizeof(uint) * drawArgs.Length, 
        ComputeBufferType.IndirectArguments
    );
    drawBuffer.SetData(drawArgs);
}

...

// GPU instancingによるレンダリングを実行する
protected void Render(float extents = 100f)
{
    block.SetBuffer("_Nodes", nodeBuffer);
    block.SetBuffer("_Edges", edgeBuffer);
    block.SetInt("_EdgesCount", edgesCount);
    block.SetMatrix("_World2Local", transform.worldToLocalMatrix);
    block.SetMatrix("_Local2World", transform.localToWorldMatrix);
    Graphics.DrawMeshInstancedIndirect(
        segment, 0, 
        material, new Bounds(Vector3.zero, Vector3.one * extents), 
        drawBuffer, 0, block
    );
}
//}

レンダリング用のシェーダ（Edge.shader）では、
Nodeの成長率パラメータ（t）に応じてEdgeの長さを制御することで、
分岐点から伸びていくブランチのアニメーションを生成しています。

//emlist[Edge.shader]{
v2f vert(appdata IN, uint iid : SV_InstanceID)
{
  v2f OUT;
  UNITY_SETUP_INSTANCE_ID(IN);
  UNITY_TRANSFER_INSTANCE_ID(IN, OUT);

  // インスタンスIDから該当するEdgeを取得
  Edge e = _Edges[iid];

  // Edgeが持つindexから2つのNodeを取得
  Node a = _Nodes[e.a];
  Node b = _Nodes[e.b];

  float3 ap = a.position;
  float3 bp = b.position;
  float3 dir = bp - ap;

  // Node bの成長率(t)に応じて、aからbへのEdgeの長さを決める
  bp = ap + normalize(dir) * length(dir) * b.t;

  // 頂点ID(IN.vid)は0か1なので、0の場合はaのNode、1の場合はbのNodeのpositionを参照する
  float3 position = lerp(ap, bp, IN.vid);

  float4 vertex = float4(position, 1);
  OUT.position = UnityObjectToClipPos(vertex);
  OUT.uv = IN.uv;

  // Nodeが不活性、またはインスタンスIDがEdgeの総数外であればalphaを0にして描画しない
  OUT.alpha = (a.active && b.active) && (iid < _EdgesCount);

  return OUT;
}
//}

これらの実装でSpace Colonization Algorithmで得られた形状をLine Topologyを用いてレンダリングすることができます。
Line.sceneを実行すると以下のような絵を得ることができます。

//image[nakamura/LineScene][Line.scene - Edge.shaderによるレンダリングの例]

==== Geometry Shaderによるレンダリング

Line TopologyのSegmentをGeometry ShaderでCapsule形状に変換することで、
厚みのある線を描けるようにします。

//image[nakamura/SegmentToCapsule][Line TopologyのSegmentをGeometryShaderでCapsule形状に変換]

頂点シェーダはEdge.shaderとほぼ変わらず、Geometry ShaderでCapsule形状を構築します。
以下では重要なGeometry Shaderの実装のみ記載します。

//emlist[TubularEdge.shader]{
...
[maxvertexcount(64)]
void geom(line v2g IN[2], inout TriangleStream<g2f> OUT) {
  v2g p0 = IN[0];
  v2g p1 = IN[1];

  float alpha = p0.alpha;

  float3 t = normalize(p1.position - p0.position);
  float3 n = normalize(p0.viewDir);
  float3 bn = cross(t, n);
  n = cross(t, bn);

  float3 tp = lerp(p0.position, p1.position, alpha);
  float thickness = _Thickness * alpha;

  // Capsuleメッシュの解像度の定義
  static const uint rows = 6, cols = 6;
  static const float rows_inv = 1.0 / rows, cols_inv = 1.0 / (cols - 1);

  g2f o0, o1;
  o0.uv = p0.uv; o0.uv2 = p0.uv2;
  o1.uv = p1.uv; o1.uv2 = p1.uv2;

  // Capsuleの側面の構築
  for (uint i = 0; i < cols; i++) {
    float r = (i * cols_inv) * UNITY_TWO_PI;

    float s, c;
    sincos(r, s, c);
    float3 normal = normalize(n * c + bn * s);

    float3 w0 = p0.position + normal * thickness;
    float3 w1 = p1.position + normal * thickness;
    o0.normal = o1.normal = normal;

    o0.position = UnityWorldToClipPos(w0);
    OUT.Append(o0);

    o1.position = UnityWorldToClipPos(w1);
    OUT.Append(o1);
  }
  OUT.RestartStrip();

  // Capsuleの先端(半球型)の構築
  uint row, col;
  for (row = 0; row < rows; row++)
  {
    float s0 = sin((row * rows_inv) * UNITY_HALF_PI);
    float s1 = sin(((row + 1) * rows_inv) * UNITY_HALF_PI);
    for (col = 0; col < cols; col++)
    {
      float r = (col * cols_inv) * UNITY_TWO_PI;

      float s, c;
      sincos(r, s, c);

      float3 n0 = normalize(n * c * (1.0 - s0) + bn * s * (1.0 - s0) + t * s0);
      float3 n1 = normalize(n * c * (1.0 - s1) + bn * s * (1.0 - s1) + t * s1);

      o0.position = UnityWorldToClipPos(float4(tp + n0 * thickness, 1));
      o0.normal = n0;
      OUT.Append(o0);

      o1.position = UnityWorldToClipPos(float4(tp + n1 * thickness, 1));
      o1.normal = n1;
      OUT.Append(o1);
    }
    OUT.RestartStrip();
  }
}

...
//}

結果は以下のようなものになります。（TubularEdge.scene）

//image[nakamura/TubularEdgeScene][TubularEdge.scene - TubularEdge.shaderによるレンダリングの例]

これでEdgeを厚みのあるMeshでレンダリングが可能になったので、
ライティングなどを施すことができるようになります。

== 応用

以上まででSpace ColonizationのGPU実装を実現できました。
本節では応用例として、スキニングアニメーションとの連携について紹介します。

この応用により、
アニメーションするモデル形状に沿ってブランチングする表現を実現することができます。

=== 大まかな流れ

スキニングアニメーションとの連携は、以下のような流れで開発します。

 1. アニメーションモデルを用意する
 2. モデルのボリュームを充填する点群を用意する（Attractionとなる点群）
 3. AttractionやNodeにBone情報を持たせる
 4. NodeにBoneの変形を適用（スキニング）して位置を変化させる

=== リソースの用意

先の例の構造体

 * Attraction
 * Node
 * Candidate

にBoneのindexを持たせるように変更を加えます。

===[column] 影響を受けるBoneの制限について

今回の応用では各Nodeについて影響するBoneの数を1つに制限しています。
本来であれば、スキニングアニメーションでは各頂点について影響するBoneの数を複数持たせることができるのですが、
この例ではシンプルに影響するBoneを1つだけに制限しています。

===[/column]

//emlist[SkinnedAttraction.cs]{
public struct SkinnedAttraction {
    public Vector3 position;
    public int bone; // boneのindex
    public int nearest;
    public uint found;
    public uint active;
}
//}

//emlist[SkinnedNode.cs]{
public struct SkinnedNode {
    public Vector3 position;
    public Vector3 animated; // スキニングアニメーション後のNodeのposition
    public int index0; // boneのindex
    public float t;
    public float offset;
    public float mass;
    public int from;
    public uint active;
}
//}

//emlist[SkinnedCandidate.cs]{
public struct SkinnedCandidate 
{
    public Vector3 position;
    public int node;
    public int bone; // boneのindex
}
//}

==== アニメーションモデルとボリューム

連携させたいアニメーションモデルを用意します。

今回の例ではClara.io@<fn>{Clara}からダウンロードしたモデルを使い（MeshLab@<fn>{MeshLab}でリダクションをかけてポリゴン数を減らしたもの）、
アニメーションはmixamo@<fn>{mixamo}で生成したものを利用しています。

//footnote[Clara][https://clara.io/view/d49ee603-8e6c-4720-bd20-9e3d7b13978a]
//footnote[MeshLab][http://www.meshlab.net/]
//footnote[mixamo][https://mixamo.com]

モデルのボリュームからAttractionの位置を取得するために、
モデルのボリューム内の点群を生成するVolumeSampler@<fn>{VolumeSampler}というパッケージを利用します。

//footnote[VolumeSampler][https://github.com/mattatz/unity-volume-sampler]

===[column] VolumeSampler

VolumeSamplerではUnity Graphics Programming vol.2「Real-Time GPU-Based Voxelizer」で解説しているテクニックを利用してモデルのボリュームを取得しています。
まず、メッシュ内部のボリュームをVoxelとして取得し、
それを元にPoisson Disk Samplingを実行することで、メッシュ内部を充填するような点群を生成しています。

VolumeSamplerを使って点群アセットを生成する方法は、
Unityツールバー内からWindow→VolumeSamplerをクリックしてWindowを表示し、
以下の図のように、

 * 対象のMeshへの参照
 * 点群のサンプリング解像度

を設定し、アセット生成ボタンをクリックすると、
指定したパスにVolumeアセットが生成されます。

//image[nakamura/VolumeSamplerWindow][VolumeSamplerWindow]

===[/column]

VolumeSamplerから生成された点群アセット（Volumeクラス）からSkinnedAttraction配列を生成し、
ComputeBufferに適用します。

//emlist[SkinnedSpaceColonization.cs]{
protected void Start() {
    ...
    // Volumeが持つ点群からSkinnedAttraction配列を生成する
    attractions = GenerateAttractions(volume); 
    count = attractions.Length;
    attractionBuffer = new ComputeBuffer(
        count, 
        Marshal.SizeOf(typeof(SkinnedAttraction)), 
        ComputeBufferType.Default
    );
    attractionBuffer.SetData(attractions);
    ...
}
//}

==== Bone情報の適用

Meshのボリュームから生成されたSkinnedAttractionには、
各位置から最も近い頂点のBone情報を適用します。

SetupSkin関数ではMeshの頂点とBoneバッファを用意し、
GPU上ですべてのSkinnedAttractionにBoneのindexを割り振ります。

//emlist[SkinnedSpaceColonization.cs]{

protected void Start() {
    ...
    SetupSkin();
    ...
}

...

protected void SetupSkin()
{
    var mesh = skinnedRenderer.sharedMesh;
    var vertices = mesh.vertices;
    var weights = mesh.boneWeights;
    var indices = new int[weights.Length];
    for(int i = 0, n = weights.Length; i < n; i++)
        indices[i] = weights[i].boneIndex0;

    using (
        ComputeBuffer
        vertBuffer = new ComputeBuffer(
            vertices.Length, 
            Marshal.SizeOf(typeof(Vector3))
        ),
        boneBuffer = new ComputeBuffer(
            weights.Length, 
            Marshal.SizeOf(typeof(uint))
        )
    )
    {
        vertBuffer.SetData(vertices);
        boneBuffer.SetData(indices);

        var kernel = compute.FindKernel("SetupSkin");
        compute.SetBuffer(kernel, "_Vertices", vertBuffer);
        compute.SetBuffer(kernel, "_Bones", boneBuffer);
        compute.SetBuffer(kernel, "_Attractions", attractionBuffer);
        GPUHelper.Dispatch1D(compute, kernel, attractionBuffer.count);
    }
}
//}

以下がGPUカーネルの実装になります。

//emlist[SkinnedSpaceColonization.compute]{
void SetupSkin (uint3 id : SV_DispatchThreadID)
{
  uint idx = id.x;
  uint count, stride;
  _Attractions.GetDimensions(count, stride);
  if (idx >= count)
    return;

  SkinnedAttraction attr = _Attractions[idx];

  // SkinnedAttractionの位置から最も近い頂点のindexを取得する
  float3 p = attr.position;
  uint closest = -1;
  float dist = 1e8;
  _Vertices.GetDimensions(count, stride);
  for (uint i = 0; i < count; i++)
  {
    float3 v = _Vertices[i];
    float l = distance(v, p);
    if (l < dist)
    {
      dist = l;
      closest = i;
    }
  }

  // 最も近い頂点のBone indexをSkinnedAttractionに設定する
  attr.bone = _Bones[closest];
  _Attractions[idx] = attr;
}
//}

=== ComputeShaderでのアルゴリズムの実装

この応用例では、
Space Colonization Algorithmにおける各ステップの中でスキニングアニメーションに必要なBone情報を取得するため、
いくつかのGPUカーネルに修正を加えます。

概ねGPUカーネルの中身は同じなのですが、
生成されるSkinnedNodeについて、最も近傍のSkinnedAttractionからBone情報（Boneのindex）を得る必要があるため、

 * Seed
 * Attract

の2つのGPUカーネルでは、近傍探索のロジックが追加されています。

//emlist[SkinnedSpaceColonization.compute]{
void Seed (uint3 id : SV_DispatchThreadID)
{
  uint idx = id.x;

  uint count, stride;
  _Seeds.GetDimensions(count, stride);
  if (idx >= count)
    return;

  SkinnedNode n;
  uint i = CreateNode(n);
  n.position = n.animated = _Seeds[idx];
  n.t = 1;
  n.offset = 0;
  n.from = -1;
  n.mass = lerp(_MassMin, _MassMax, nrand(id.xy));

  // 最近傍のSkinnedAttractionを探索し、
  // Bone indexをコピーする
  uint nearest = -1;
  float dist = 1e8;
  _Attractions.GetDimensions(count, stride);
  for (uint j = 0; j < count; j++)
  {
    SkinnedAttraction attr = _Attractions[j];
    float l = distance(attr.position, n.position);
    if (l < dist)
    {
      nearest = j;
      dist = l;
    }
  }
  n.index0 = _Attractions[nearest].bone;

  _Nodes[i] = n;
}

...

void Attract (uint3 id : SV_DispatchThreadID)
{
  uint idx = id.x;
  uint count, stride;
  _Nodes.GetDimensions(count, stride);
  if (idx >= count)
    return;

  SkinnedNode n = _Nodes[idx];

  if (n.active && n.t >= 1.0)
  {
    float3 dir = (0.0).xxx;
    uint counter = 0;

    float dist = 1e8;
    uint nearest = -1;

    _Attractions.GetDimensions(count, stride);
    for (uint i = 0; i < count; i++)
    {
      SkinnedAttraction attr = _Attractions[i];
      if (attr.active && attr.found && attr.nearest == idx)
      {
        float3 dir2 = (attr.position - n.position);
        dir += normalize(dir2);
        counter++;

        // 最近傍のSkinnedAttractionを探索する
        float l2 = length(dir2);
        if (l2 < dist)
        {
          dist = l2;
          nearest = i;
        }
      }
    }

    if (counter > 0)
    {
      SkinnedCandidate c;
      dir = dir / counter;
      c.position = n.position + (dir * _GrowthDistance);
      c.node = idx;
      // 最近傍のSkinnedAttractionの持つBone indexを設定する
      c.bone = _Attractions[nearest].bone;
      _CandidatesAppend.Append(c);
    }
  }
}

//}

==== スキニングアニメーション

以上までの実装で、Nodeに対してBone情報を設定しながらSpace Colonization Algorithmを実行できるようになりました。

あとは必要なBone行列をSkinnedMeshRendererから取得し、
GPU上でSkinnedNodeの位置をBoneの変形に応じて動かすことで、Nodeに対してスキニングアニメーションを施すことができます。

//emlist[SkinnedSpaceColonization.cs]{

protected void Start() {
    ...
    // bind pose行列のバッファを作成しておく
    var bindposes = skinnedRenderer.sharedMesh.bindposes;
    bindPoseBuffer = new ComputeBuffer(
        bindposes.Length, 
        Marshal.SizeOf(typeof(Matrix4x4))
    );
    bindPoseBuffer.SetData(bindposes);
    ...
}

protected void Animate()
{
    // アニメーションが再生されることで更新される、SkinnedMeshRendererのBone行列を表すバッファを作成する
    var bones = skinnedRenderer.bones.Select(bone => {
        return bone.localToWorldMatrix;
    }).ToArray();
    using (
        ComputeBuffer boneMatrixBuffer = new ComputeBuffer(
            bones.Length, 
            Marshal.SizeOf(typeof(Matrix4x4))
        )
    )
    {
        boneMatrixBuffer.SetData(bones);

        // BoneとNodeのバッファを渡し、GPUスキニングを実行する
        var kernel = compute.FindKernel("Animate");
        compute.SetBuffer(kernel, "_BindPoses", bindPoseBuffer);
        compute.SetBuffer(kernel, "_BoneMatrices", boneMatrixBuffer);
        compute.SetBuffer(kernel, "_Nodes", nodeBuffer);
        GPUHelper.Dispatch1D(compute, kernel, count);
    }
}
//}

//emlist[SkinnedSpaceColonization.compute]{
void Animate (uint3 id : SV_DispatchThreadID)
{
  uint idx = id.x;
  uint count, stride;
  _Nodes.GetDimensions(count, stride);
  if (idx >= count)
    return;

  SkinnedNode node = _Nodes[idx];
  if (node.active)
  {
    // スキニングの実行
    float4x4 bind = _BindPoses[node.index0];
    float4x4 m = _BoneMatrices[node.index0];
    node.animated = mul(mul(m, bind), float4(node.position, 1)).xyz;
    _Nodes[idx] = node;
  }
}
//}

=== レンダリング

レンダリング用のシェーダはほぼ同じなのですが、
SkinnedNodeの元の位置（position）ではなく、
スキニングアニメーションを施した後の位置（animated）を参照してEdgeを描画する点だけが異なります。

//emlist[SkinnedTubularEdge.hlsl]{
v2g vert(appdata IN, uint iid : SV_InstanceID)
{
  ...
  Edge e = _Edges[iid];

  // スキニングアニメーションを施した後の位置を参照する
  SkinnedNode a = _Nodes[e.a], b = _Nodes[e.b];
  float3 ap = a.animated, bp = b.animated;

  float3 dir = bp - ap;
  bp = ap + normalize(dir) * length(dir) * b.t;
  float3 position = lerp(ap, bp, IN.vid);
  OUT.position = mul(unity_ObjectToWorld, float4(position, 1)).xyz;
  ...
}
//}

以上までの実装で冒頭に示したキャプチャの絵を得ることができます。（@<img>{nakamura/SkinnedAnimationScene} SkinnedAnimation.scene）

== まとめ

本章では、
点群に沿ってブランチング形状を生成するSpace Colonization AlgorithmのGPU実装と、
スキニングアニメーションと組み合わせる応用例を紹介しました。

この手法では、

 * Attractionの影響範囲（influence distance）
 * Nodeが伸びる長さ（growth distance）
 * Attractionを削除する範囲（kill distance）

という3つのパラメータで枝の粗密具合をコントロールできますが、
さらにこれらのパラメータを局所的に変えたり、時間によって変化させたりすることで、より多彩なモデルを生成することができます。

また、サンプルではAttractionは初期化の際に生成したものだけでアルゴリズムを実行していますが、
Attractionを動的に増やすことでもより多くの異なるパターンを生成することができるはずです。

興味のある方は本アルゴリズムの様々な応用を試して面白いパターンを探してみてください。

== 参考

 * Modeling Trees with a Space Colonization Algorithm - http://algorithmicbotany.org/papers/colonization.egwnp2007.large.pdf
 * Algorithmic Design with Houdini - https://vimeo.com/305061631#t=1500s


