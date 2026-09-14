//
//  ContentView.swift
//  Ori
//
//  Created by 今大輔 on 2026/09/13.
//

//import SwiftUI

//struct ContentView: View {
//    var body: some View {
//        VStack {
//            Image(systemName: "globe")
//                .imageScale(.large)
//                .foregroundStyle(.tint)
//            Text("Hello, world!")
//        }
//        .padding()
//    }
//}
//
//#Preview {
//    ContentView()
//}

import SwiftUI

struct ContentView: View {
    var body: some View {
        Text("\(ori_ping())")
            .font(.system(size: 80))
    }
}
