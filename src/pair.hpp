namespace sys {
  template <typename A, typename B>
    struct Pair {
      A one;
      B two;

      Pair(A a, B b)
        : one(a)
        , two(b)
      {}

      u32 hash() {
        return one ^ two;
      }

      bool operator==(Pair<A,B>& other) {
        return one == other.one && two == other.two;
      }
    };

  template <typename A, typename B>
    Pair<A,B> pair(A a, B b) {
      return Pair<A,B>(a, b);
    }
}
