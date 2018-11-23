<?php
/**
 * 闭包是由匿名函数（也成闭包函数）构成的一个整体，和普通的匿名函数有所不同，闭包中一定存在引用了外部数据并在内部操作的情况。
 */

$pipes = [
    function ($passable, $next) {
        echo 111, PHP_EOL;
        return $next($passable);
    },
    function ($passable, $next) {
        echo 222, PHP_EOL;
        return $next($passable);
    },
    function ($passable, $next) {
        echo 333, PHP_EOL;
        return $next($passable);
    },
];

$destination = function ($passable) {
    echo '<destination>', PHP_EOL;
    return $passable + 100;
};

$initial = function ($passable) use ($destination) {
    return $destination($passable);
};

$stack = $initial;
foreach (array_reverse($pipes) as $pipe) {
    $stack = function ($passable) use ($stack, $pipe) {
        return $pipe($passable, $stack);
    };
}

$stack(0);

// -------------------------------------------------------------------------------
// 最终的结果相当于如下调用：
$passable = null;

$pipes[0]($passable, function ($passable) use ($pipes) {
    return $pipes[1]($passable, function ($passable) use ($pipes) {
        return $pipes[2]($passable, function ($passable) {
            echo '<destination>', PHP_EOL;
            return $passable + 100;
        });
    });
});
/*-------------------------------------------------------------------------------
|
| 注意要点：
| （1）这里其实是构造一个“递归”的闭包调用栈，最后，一次性反向调用这个闭包栈。
|      因为调用要从数组的第一个元素开始，所以，封装入栈的时候要 array_reverse 一次。
|      同理，destination 当然要作为 array_reduce 的初始值，被封装到最底部。
| （2）传递的参数 $pipe 必须具有两个参数，且必须调用第二个参数上的闭包。否则，闭包栈不能继续往下进行。
|      例如： function($passable, $callback) { ...; return $callback($passable); }
|
|--------------------------------------------------------------------------------
*/
